
#include <iostream>
#include <cassert>
#include <string>
#include <fstream>
#include <iterator>
#include <algorithm>
#include "encoder.h"
#include <unordered_set>
#include <unordered_map>
#include <stdexcept>

static const bool LOG_VERBOSE = std::getenv("LOG_VERBOSE")?
        atoi(std::getenv("LOG_VERBOSE")) == 1 : false;

//TODO Design Docs and documenataion

/***************************************directory_reader****************************************/
directory_reader::directory_reader(const fs::path& dir_path, std::string fext) :
        dir_path(dir_path), file_ext(fext) {

    try {
        if (fs::exists(dir_path) && fs::is_directory(dir_path) && !dir_path.empty()) {
            if (!all_files.empty())
                all_files.clear();
            add_files_with_ext();
        }

    } catch (std::exception& ex) {
        if (LOG_VERBOSE)
            std::cout << __PRETTY_FUNCTION__ << " " << ex.what() << std::endl;
        throw;
    }
}

void directory_reader::print_files() {
    for (auto& filename : all_files)
        std::cout << filename << "\n";
}

fs::path directory_reader::get_a_file() {
    std::lock_guard<std::mutex> lock(mtx);

    if (all_files.empty())  //Note: Can be modified to throw exception
        return fs::path();
    std::string wfile = all_files.back();
    all_files.pop_back();
    auto full_path = dir_path / wfile;
    return full_path;
}

std::vector<std::string> directory_reader::get_all_files() {
    std::lock_guard<std::mutex> lock(mtx);
    return std::move(all_files);
}
bool directory_reader::do_file_match_ext(fs::path& dir_path) {
    if (dir_path.extension() == file_ext)
        return true;
    else
        return false;
}
void directory_reader::add_files_with_ext() {
    for (const auto& entry : fs::directory_iterator(dir_path)) {
        auto filename = entry.path().filename();
        if (do_file_match_ext(filename))
            all_files.push_back(filename.string());
    }

}


/***************************************wave_header****************************************/
wave_header::wave_header(const fs::path& wav_file) {
    try {
        if (fs::exists(wav_file) && !fs::is_directory(wav_file) && fs::is_regular_file(wav_file)) {
            uptr_wave_hdr = std::make_unique<wave_file_header>();
            std::ifstream fstrm(wav_file.string());
            wave_file_header* hdr = uptr_wave_hdr.get();
            fstrm.read((char*) hdr, sizeof(wave_file_header));
            //Note: depending on audio_format(PCM, etc), header length can vary
        }
    } catch (std::exception& ex) {
        if (LOG_VERBOSE)
            std::cout << __PRETTY_FUNCTION__ << " " << ex.what() << std::endl;
        throw;
    }
}

void wave_header::print_header() const{
    std::cout << "RIFF Chunk Tag             :" << uptr_wave_hdr->riff_tag[0] << uptr_wave_hdr->riff_tag[1]
                                       << uptr_wave_hdr->riff_tag[2] << uptr_wave_hdr->riff_tag[3] << std::endl;
    std::cout << "RIFF Chunk size            :" << uptr_wave_hdr->chunk_size << std::endl;
    std::cout << "WAVE Tag                   :" << uptr_wave_hdr->wave_tag[0] << uptr_wave_hdr->wave_tag[1]
                                       << uptr_wave_hdr->wave_tag[2] << uptr_wave_hdr->wave_tag[3] << std::endl;
    std::cout << "Subchunk1 FMT Tag          :" << uptr_wave_hdr->fmt[0] << uptr_wave_hdr->fmt[1]
                                         << uptr_wave_hdr->fmt[2] << uptr_wave_hdr->fmt[3] << std::endl;
    std::cout << "Subchunk1 length           :" << uptr_wave_hdr->subchunk1_size << std::endl;
    std::cout << "Sampling Rate              :" << uptr_wave_hdr->samples_per_sec << std::endl;
    std::cout << "Number of bits used        :" << uptr_wave_hdr->bits_per_sample <<std::endl;
    std::cout << "Number of channels         :" << uptr_wave_hdr->num_channel << std::endl;
    std::cout << "Number of bytes per second :" << uptr_wave_hdr->bytes_per_sec << std::endl;
    std::cout << "Audio Format               :" << uptr_wave_hdr->audio_format << std::endl;
    std::cout << "Block align                :" << uptr_wave_hdr->block_align << std::endl;
    std::cout << "Subchunk2(Data) tag        :" << uptr_wave_hdr->subchunk2_tag[0] <<
            uptr_wave_hdr->subchunk2_tag[1] << uptr_wave_hdr->subchunk2_tag[2] << uptr_wave_hdr->subchunk2_tag[3] <<std::endl;
    std::cout << "Subchunk2(Data) len        :" << uptr_wave_hdr->subchunk2_size << std::endl;
}

int  wave_header::num_samples() const {
    return  uptr_wave_hdr->chunk_size / bytes_per_sample();
}
int  wave_header::num_channel() const{
    return uptr_wave_hdr->num_channel;
}
int wave_header::get_sampling_rate() const{
    return uptr_wave_hdr->samples_per_sec;
}
int  wave_header::bytes_per_sample() const{
    return uptr_wave_hdr->bits_per_sample/bits_in_byte;
}
int wave_header::file_size() const{
    return uptr_wave_hdr->chunk_size + riff_desc_len;
}
/***************************************mp3_encoder****************************************/
struct inout_stream {
    std::ifstream rd;
    std::ofstream wr;
    std::error_code open(const fs::path& wpath) {

        std::string out_filename = wpath.stem().string() + ".mp3";
        fs::path out_filepath = wpath.parent_path() / out_filename;

        rd.open(wpath.string());
        if (!rd.good()) {
            return std::error_code(errno, std::generic_category());
        }

        wr.open(out_filepath.string());
        if (!wr.good()) {
            return std::error_code(errno, std::generic_category());
        }
        if (LOG_VERBOSE) {
            std::cout << "Read " << wpath.string() << "\n";
            std::cout << "Write " << out_filepath.string() << "\n";
        }
        return std::error_code { };
    }

};



mp3_encoder::mp3_encoder() :
        lame(nullptr) {
    lame = lame_init();
    if(lame == nullptr)
        throw std::runtime_error("mp3_encoder Constructor failed");
}

mp3_encoder::~mp3_encoder() {
    lame_close(lame);
}

std::string mp3_encoder::get_lame_version() {
    lame_version_t ver_num;
    get_lame_version_numerical(&ver_num);

    std::string ver = std::to_string(ver_num.major) + std::string(".") +std::to_string(ver_num.minor)
    + std::string(".") +std::to_string(ver_num.alpha);
    //+std::string(ver_num.features);
    if(LOG_VERBOSE)
        printf(" %d %d %s \n", ver_num.major, ver_num.minor, ver_num.features);
    return ver;
}

void mp3_encoder::print_lame_config() {
    printf(" -------------------\n");
    lame_print_config(lame);
    if(param_initialized)
        lame_print_internals(lame);
}


int mp3_encoder::num_bytes_to_read(wave_header& header, int total_read) {
    int bytes_per_frame = header.num_channel() * header.bytes_per_sample();
    auto bytes_to_read = wav_buf_len * bytes_per_frame;

    int num_char_to_read = 0;
    if (total_read + bytes_to_read > header.file_size())
        num_char_to_read = header.file_size() - total_read;
    else
        num_char_to_read = bytes_to_read;

    return num_char_to_read;
}

void mp3_encoder::split_buf(std::vector<short int>& buf, std::vector<short int>& buf_l,
        std::vector<short int>& buf_r) {
    for (int i = 0; i < buf.size(); i++) {
        if (i % 2 == 0)
            buf_l.push_back(buf[i]);
        else
            buf_r.push_back(buf[i]);
    }
}

std::error_code mp3_encoder::encode_wav(const fs::path& wpath, bool interlived) {
    std::error_code err = validate_path(wpath);
    if (err)
        return err;

    inout_stream file_pair;
    err = file_pair.open(wpath);
    if (err)
        return err;

    wave_header header(wpath);
    int bytes_per_frame = header.num_channel() * header.bytes_per_sample();
    int fsize = header.file_size();
    int total_read = 0;

    if(!param_initialized){
        encoder_param_config config;
        init_params(config);
    }
    param_initialized = true;

    if(LOG_VERBOSE)
        std::cout <<"encode_wav: thread ID "<< std::this_thread::get_id() <<" " << wpath;

    while (1) {
        std::vector<short int> wav_buffer(wav_buf_len * 2);
        std::vector<unsigned char> mp3_buffer(mp3_buf_len);
        std::vector<short int> wav_buffer_l, wav_buffer_r;
        //read from file into buffer
        auto num_char_to_read = num_bytes_to_read(header, total_read);
        file_pair.rd.read((char*) wav_buffer.data(), num_char_to_read);
        int num_char_read = file_pair.rd.gcount();
        total_read += num_char_read;

        int num_char_written = 0;
        if (num_char_read == 0)
            num_char_written = lame_encode_flush(lame, mp3_buffer.data(), mp3_buf_len);
        else {
            if (interlived) {
                num_char_written = lame_encode_buffer_interleaved(lame, wav_buffer.data(),
                        num_char_read / bytes_per_frame, mp3_buffer.data(), mp3_buf_len);
            }
            else {
                split_buf(wav_buffer, wav_buffer_l, wav_buffer_r);
                num_char_written = lame_encode_buffer(lame, wav_buffer_l.data(),
                        wav_buffer_r.data(), num_char_read / bytes_per_frame, mp3_buffer.data(),
                        mp3_buf_len);
            }
        }

        file_pair.wr.write((char*) mp3_buffer.data(), num_char_written);

        if(LOG_VERBOSE)
            std::cout  <<"("<<num_char_read<< ", " << num_char_written <<") ";

        if (!num_char_read)
            break;

    }
    if(LOG_VERBOSE)
        std::cout  << total_read <<"\n";
    return std::error_code { };
}
//TODO convert into custom error code and return std::error_code
int mp3_encoder::init_params(const encoder_param_config& config) {
    int err = lame_set_in_samplerate(lame, config.sample_rate);
    if(err < 0)
        return err;
    err = lame_set_VBR(lame, config.mode);
    if(err < 0)
            return err;

    err = lame_set_quality(lame, config.quality);
    if(err < 0)
            return err;

    lame_set_brate(lame,config.brate);

    err = lame_init_params(lame);
    if(err < 0)
        return err;

    param_initialized = true;
    return 0;
}

std::error_code mp3_encoder::validate_path(const fs::path& wpath) {
    std::error_code err { };
    fs::exists(wpath, err);
    if (err)
        return err;

    fs::is_regular_file(wpath, err);
    if (err)
        return err;

    fs::permissions(wpath, fs::perms::owner_all | fs::perms::group_all, err);
    if (err)
        return err;

    return err;
}

/***************************************encoder_param_config****************************************/
void encoder_param_config::setmode(vbr_mode mode) {
    this->mode = mode;
}

int encoder_param_config::setsample_rate(int sampleRate) {
    std::unordered_set<int> valid_rate = {44100, 22050,11025,
            48000, 24000,  12000, 32000, 16000, 8000};
    if(valid_rate.find(sampleRate) != valid_rate.end()){
        sample_rate = sampleRate;
        return 0;
    }
    return -1;
}

int encoder_param_config::setbrate(int brate) {
    if (brate< 320) {
        this->brate = brate;
        return 0;
    }
    return -1;
}

int encoder_param_config::setquality(int quality) {
    std::unordered_set<int> valid_q = {2,4,6,8,10};
    if (valid_q.find(quality) != valid_q.end()) {
        this->quality = quality;
        return 0;
    }
    return -1;

}

/***************************************concurrent_encoder****************************************/
int concurrent_encoder::encode_thread_task(const encoder_param_config& config) {
    mp3_encoder enc;
    int err = enc.init_params(config);
    if(err)
        throw std::runtime_error("lame init param failed");

    while (1) {
        auto wav_path = file_queue.pop();
        if(LOG_VERBOSE)
            std::cout << std::this_thread::get_id() << "Encoding file " << wav_path.string() << "\n";
        if (wav_path.string() == "-1")
            break;

        if (!fs::exists(wav_path) && !fs::is_regular_file(wav_path))
            continue;

        enc.encode_wav(wav_path);
    }
    return 0;
}

int concurrent_encoder::create_threads(const encoder_param_config& config, int num_thread_usr) {

    int curr_num_thread = thread_pool.size();

    int num_core = std::thread::hardware_concurrency();
    int req_thread = std::min(num_core-curr_num_thread, num_thread_usr+curr_num_thread);

    int j = 0;
    while (j < req_thread) {
        auto t1 = std::thread(&concurrent_encoder::encode_thread_task, this, std::ref(config));
        thread_pool.push_back(std::move(t1));
        j++;
    }
    if(LOG_VERBOSE)
        std::cout << " Num of thread created" << req_thread << " Max Optimum thread " << num_core << "\n";
    thread_created = true;
    return j;
}

int concurrent_encoder::encode_wav_files(fs::path& pth) {

    std::error_code err = validate_path(pth);
    if(err){
        throw std::system_error(err);
    }
    directory_reader dr(pth);
    if(LOG_VERBOSE)
        dr.print_files();
#if 0
    while(1) {
        auto wf = dr.get_a_file();
        if(wf.empty())
        break;
        file_queue.push(wf);
    }
#endif
    int num_file = 0;
    auto wav_files = dr.get_all_files();
    for (auto& file_name : wav_files){
        auto fpath = dr.full_path(file_name);
        file_queue.push(fpath);
        num_file++;
    }
    if(!thread_created){
        encoder_param_config config;
        create_threads(config);
    }

    file_queue.wait_until_empty();

    return num_file;

}

bool concurrent_encoder::done_all() {
    if (file_queue.empty())
        return true;
    return false;
}
int concurrent_encoder::join_all() {

    int num_th = thread_pool.size();

    for (int i = 0; i < num_th; i++) {
        file_queue.push(fs::path("-1"));
    }
    for (int i = 0; i < num_th; i++) {
        if (thread_pool[i].joinable())
            thread_pool[i].join();
    }
    thread_pool.clear();
    thread_created = false;
    return num_th;
}
std::error_code concurrent_encoder::validate_path(fs::path& wpath) {
    std::error_code err { };
    fs::exists(wpath, err);
    if (err)
        return err;

    fs::is_directory(wpath, err);
    if (err)
        return err;

    fs::permissions(wpath, fs::perms::owner_all | fs::perms::group_all, err);
    if (err)
        return err;

    return err;
}
concurrent_encoder::~concurrent_encoder(){
    if(thread_created)
        join_all();
}
/***************************************encoder cli**************************************************/

class arg_parser{
    std::unordered_set<std::string> arg_name = {"thread", "brate", "sample_rate", "quality", "dir_path"};
    std::unordered_map<std::string, std::string> args;
public:
    void  parse(int argc, char** argv) {
        if(argc%2 ==1)
            throw std::invalid_argument(" Incorrect options ");

        int i = 1;
        while (i < argc - 1) {
            if (arg_name.find(argv[i]) != arg_name.end()) {
                args[argv[i]] = argv[i + 1];
                i = i + 2;
            }
            else
               throw std::invalid_argument(" Incorrect options ");
        }
        args["dir_path"] = argv[i];
    }

    int get(std::string arg_name1){
        if(arg_name1 == "dir_path")
            return -1;
        if(args.find(arg_name1) != args.end()){
            return stoi(args[arg_name1]);
        }
        else
            return -1;
    }

    std::string get_dir_path(){
        return args["dir_path"];
    }


    void get_args(encoder_param_config& conf, int& num_thread,
            std::string& dir_path) {
        int val = this->get("thread");
        if (val > 0) {
            int num_core = std::thread::hardware_concurrency();
            num_thread = val;
            if(num_thread > num_core)
                throw std::invalid_argument(" user thread greater then number of core ("+ std::to_string(num_core)+")");
        }

        val = this->get("brate");
        if (val > 0) {
            int err = conf.setbrate(val);
            if(err)
                throw std::invalid_argument(" invalid  brate ");
        }

        val = this->get("sample_rate");
        if (val > 0) {
            int err = conf.setsample_rate(val);
            if(err)
                throw std::invalid_argument(" invalid  sample_rate ");
        }

        val = this->get("quality");
        if (val > 0) {
            int err = conf.setquality(val);
            if(err)
                throw std::invalid_argument(" invalid  quality ");
        }
        dir_path = this->get_dir_path();
    }
};


int encoder_cli::simple_main(int argc, char** argv) {
    if (argc < 2) {
        usages();
        return 0;
    }
    try {
        fs::path dir_path(argv[1]);
        concurrent_encoder conc_enc;
        int num_file = conc_enc.encode_wav_files(dir_path);
        std::cout << " Encoded " << num_file << " wav files to mp3 from directory " << dir_path
                << "\n";
    } catch (std::exception& exc) {
        std::cout << exc.what() << "\n";
    }
    return 0;
}
int encoder_cli::main(int argc, char** argv) {
    if (argc <= 2) {
        return simple_main(argc, argv);
    }

    encoder_param_config conf;
    int num_thread = 3;
    std::string dir_name;
    arg_parser arg_par;
    try {
        arg_par.parse(argc, argv);
        arg_par.get_args(conf, num_thread, dir_name);
        fs::path dir_path(dir_name);
        concurrent_encoder conc_enc;
        conc_enc.create_threads(conf, num_thread);
        int num_file = conc_enc.encode_wav_files(dir_path);
        std::cout << " Encoded " << num_file << " wav files to mp3 from directory " << dir_path
                << "\n";
    }  catch (std::invalid_argument& ex) {
        std::cout << ex.what() << "\n";
        usages();
    }
    catch (std::exception& exc) {
        std::cout << exc.what() << "\n";
    }
    return 0;
}
/***************************************end**************************************************/

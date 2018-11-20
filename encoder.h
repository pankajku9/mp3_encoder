/*
 * encoder.h
 *
 *  Created on: Nov 15, 2018
 *      Author: pankajku
 */

#ifndef ENCODER_H_
#define ENCODER_H_

#if 1
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <iostream>
#include <system_error>
#include "lame.h"


/* Reads all the  files with extension (default .wav) from a given directory,
 *  throws exception if invalid path
 */
class directory_reader {
public:
    directory_reader(const fs::path& dir_path, std::string file_ext = ".wav");
    void print_files();
    fs::path get_a_file();
    std::vector<std::string> get_all_files();
    inline int get_num_file(){ return all_files.size(); }
    inline fs::path full_path(std::string& file_name){
        return  dir_path / fs::path(file_name);
    }
private:
    bool do_file_match_ext(fs::path& dir_path);
    void add_files_with_ext();
    directory_reader(directory_reader&) = delete;
    std::mutex mtx;
    const fs::path dir_path;
    std::vector<std::string> all_files;
    std::string file_ext;
};

/* Wav file format header info representation */
struct wave_file_header {
    /* RIFF Chunk Descriptor */
    uint8_t riff_tag[4];
    uint32_t chunk_size;
    /* "fmt" sub-chunk */
    uint8_t wave_tag[4];
    uint8_t fmt[4];
    uint32_t subchunk1_size;
    uint16_t audio_format;
    uint16_t num_channel;
    uint32_t samples_per_sec;
    uint32_t bytes_per_sec;
    uint16_t block_align;
    uint16_t bits_per_sample;
    /* "data" sub-chunk */
    uint8_t subchunk2_tag[4];
    uint32_t subchunk2_size;
};

/* Read header info from a wave file and abstracts parameter access*/
class wave_header {
    std::unique_ptr<wave_file_header> uptr_wave_hdr;
    const int bits_in_byte = 8;
    const int riff_desc_len = 8;
public:
    wave_header(const fs::path& wav_file);
    void print_header() const;
    int num_samples() const;
    int num_channel() const;
    int bytes_per_sample() const;
    int file_size() const;
    int get_sampling_rate() const;
};

struct encoder_param_config{
    int sample_rate = 44100;
    int quality = 2;
    int brate = 128;
    vbr_mode mode = vbr_default;
    void setmode(vbr_mode mode = vbr_default);
    int setsample_rate(int sampleRate = 44100);
    int setbrate(int brate = 128);
    int setquality(int quality = 2) ;
    //*note this class can be extended to support any number of lame parameter
};

/* Encode a  wav file into mp3*/
class mp3_encoder {
public:
    mp3_encoder();
    ~mp3_encoder();
    int init_params(const encoder_param_config& config);
    std::error_code encode_wav(const fs::path& file_path, bool interlived = true);
    void print_lame_config();
    std::string get_lame_version();
    int getmp3_buf_len() const {return mp3_buf_len;};
    int getwav_buf_len() const{return wav_buf_len;};
    void setmp3_buff_len(std::size_t buf_len);
    int setwav_buff_len(std::size_t buf_len);
private:
    int num_bytes_to_read(wave_header& header, int total_read);
    std::error_code validate_path(const fs::path& file_path);
    void split_buf(std::vector<short int>& buf, std::vector<short int>& buf_l,
            std::vector<short int>& buf_r);
    const std::size_t wav_buf_len = 8192;
    const std::size_t mp3_buf_len = 8192;
    bool param_initialized = false;
    lame_t lame;
};

/*
 *  A thread safe queue of to hold a type T.
 * pop return the front element or  will wait if queue is empty
 *  push adds to queue and signal one process if any is waiting */
template<typename T = int>
class wait_notify_queue {
    std::queue<T> que;
    std::mutex mtx, mtx2;
    std::condition_variable condvar, condvar2;
public:
    void push(T strmID) {
        std::lock_guard<std::mutex> locker(mtx);
        que.push(strmID);
        condvar.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> locker(mtx);
        if(que.empty())
            condvar2.notify_one();
        condvar.wait(locker, [this] {return !que.empty();});
        T strm = que.front();
        que.pop();
        return strm;
    }

    void wait_until_empty(){
        std::unique_lock<std::mutex> locker(mtx2);
        condvar2.wait(locker, [this] {return que.empty();});
    }

    bool empty() {
        return que.empty();
    }
};

/* Encode all the wave in the directory using user configured number of thread*/
class concurrent_encoder {
public:
    concurrent_encoder() {};
    int create_threads(const encoder_param_config& conf, int num_thread_usr = 3);
    bool done_all();
    int join_all();
    int encode_wav_files(fs::path& dir_path);
    ~concurrent_encoder();
private:
    int encode_thread_task(const encoder_param_config& conf);
    std::error_code validate_path(fs::path& file_path);
    std::vector<std::thread> thread_pool;
    wait_notify_queue<fs::path> file_queue;
    bool thread_created = false;
};
/* Handles command line based input for concurrent_encoder*/
class encoder_cli {
public:
    int simple_main(int argc,  char** argv);
    int main(int argc,  char** argv);
    void usages(){
        std::cout<<"./encode [options] directory_path \n Options";
        std::cout<<"\n  thread      <n> : number of thread to execute"\
                   "\n  brate       <n> : bit rate for encoder"\
                   "\n  sample_rate <n> : sample rate for encoder" \
                   "\n  quality     <n> : quality for encoder\n";
    }
};
#endif /* ENCODER_H_ */

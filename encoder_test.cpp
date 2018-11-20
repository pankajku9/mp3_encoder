/*
 * encoder_test.cpp
 *
 *  Created on: Nov 15, 2018
 *      Author: pankajku
 */

#include "encoder.h"
#include <cassert>
#include <iostream>
#include <regex>
#include <string>
#include <cstring>

static const bool LOG_VERBOSE =
        std::getenv("LOG_VERBOSE") ? atoi(std::getenv("LOG_VERBOSE")) == 1 : false;

static const bool TEST_REPORT_LOG =
        std::getenv("TEST_REPORT_LOG") ? atoi(std::getenv("TEST_REPORT_LOG")) == 1 : false;

static int global_tc_number______ = 1;

#define log_test_report_pass()  if(TEST_REPORT_LOG) \
                    std::cout <<global_tc_number______++ <<" : "<< __PRETTY_FUNCTION__ << " Passed \n"

/* There is test class corresponding to every class in header file
 * test_directory_reader - directory_reader
 * test_wave_header - wave_header
 * test_mp3_encoder - mp3_encoder
 * test_wait_notify_que -wait_notify_que
 * test_concurrent_thread - concurrent_thread
 * */

/* Note1: For simplicity I am using assert which can be easily
 * replaced with unit test framework call or custom assert macro
 * if requirement is not to break the flow and get overall result
 *
    #define assert(exp) if(!exp) return;

 */

static auto test_data_dir_path = fs::current_path() / "test_data";
static auto test_file_path1 = fs::current_path() / "test_data/testcase.wav";


class test_directory_reader {
public:
    static void test() {
        test_directory_reader tdr;
        tdr.tc0();
        tdr.tc1();
        tdr.tc2();
        tdr.tc3();
        tdr.tc4();
    }

private:
    void tc0() { // bad path
        const fs::path dir_path { "/asd/" };
        try {
            directory_reader dr(dir_path);
        } catch (std::system_error& err) {
            if (LOG_VERBOSE)
                std::cout << err.what() << std::endl;
            assert(err.code() == std::errc::no_such_file_or_directory);
        }
        log_test_report_pass();
    }
    void tc1() { // empty path
        const fs::path dir_path { };
        try {
            directory_reader dr(dir_path);
        } catch (std::system_error& err) {
            if (LOG_VERBOSE)
                std::cout << err.what() << std::endl;
            assert(err.code() == std::errc::no_such_file_or_directory);
        }
        log_test_report_pass();
    }
    void tc2() {
        const fs::path dir_path { test_data_dir_path };
        directory_reader dr(dir_path);
        std::regex txt_regex("[a-z0-9]+\\.wav");
        auto temp = dr.get_a_file();
        assert(fs::exists(temp));
        assert(std::regex_match(temp.filename().string(), txt_regex));
        log_test_report_pass();
    }
    void tc3() {
        const fs::path dir_path { test_data_dir_path };
        directory_reader dr(dir_path);
        assert(4 == dr.get_num_file()); //TODO
        std::regex txt_regex("[a-z0-9]+\\.wav");
        auto file_list = dr.get_all_files();
        for (auto& fname : file_list) {
            if (LOG_VERBOSE)
                std::cout << " " << fname;
            assert(std::regex_match(fname, txt_regex));
        }
        log_test_report_pass();
    }

    void tc4() {
        std::string str = (test_data_dir_path ).string();
        directory_reader dr(str);
        if (LOG_VERBOSE)
            dr.print_files();
        log_test_report_pass();
    }

};

class test_wave_header {
public:
    static void test() {
        test_wave_header twh;
        twh.tc0();
        twh.tc1();
        twh.tc2();
        twh.tc3();
    }
private:

    void tc0() { // bad path
        const fs::path file_path { "/asd/" };
        try {
            wave_header hdr(file_path);
        } catch (std::system_error& err) {
            if (LOG_VERBOSE)
                std::cout << err.what() << std::endl;
            assert(err.code() == std::errc::no_such_file_or_directory);
        }
        const fs::path file_path1 { };
        try {
            wave_header hdr(file_path1);
        } catch (std::system_error& err) {
            if (LOG_VERBOSE)
                std::cout << err.what() << std::endl;
            assert(err.code() == std::errc::no_such_file_or_directory);
        }
        log_test_report_pass();

    }
    void tc1() {
        auto pth = fs::current_path();
        try {
            wave_header hdr(pth);
        } catch (std::runtime_error& err) {
            if (LOG_VERBOSE)
                std::cout << err.what() << std::endl;
            assert(std::strcmp(err.what(), "Not a regular file") == 0); //TODO define custom error code
        }
        log_test_report_pass();
    }

    void tc2() {
        wave_header hdr(test_file_path1);
        if (LOG_VERBOSE)
            hdr.print_header();
        log_test_report_pass();
    }

    void tc3() {
        wave_header hdr(test_file_path1);
        assert(hdr.num_channel() == 2);
        assert(hdr.file_size() == fs::file_size(test_file_path1));
        assert(44100 == hdr.get_sampling_rate());
        log_test_report_pass();
    }
};

class test_mp3_encoder {

public:

    static void test() {
        test_mp3_encoder tmp3_enc;
        tmp3_enc.tc0();
        tmp3_enc.tc1();
        tmp3_enc.tc2();
        tmp3_enc.tc3();
        tmp3_enc.tc4();

    }
private:

    void tc0() {
        try {
            mp3_encoder enc;
            auto ver = enc.get_lame_version();
            assert(ver == "3.100.0");
            if (LOG_VERBOSE)
                enc.print_lame_config();
        } catch (std::runtime_error& err) {
            if (LOG_VERBOSE)
                std::cout << err.what() << std::endl;
        }
        log_test_report_pass();
    }

    void tc1() {
        mp3_encoder enc;
        encoder_param_config config;
        int err = config.setsample_rate(44100);
        assert(err == 0);

        err = config.setsample_rate(0);
        assert(err != 0);

        err = config.setquality(2);
        assert(err == 0);

        err = config.setquality(12);
        assert(err != 0);

        err = config.setbrate(128);
        assert(err == 0);

        err = config.setbrate(500);
        assert(err != 0);

        err = enc.init_params(config);
        assert(err == 0);
        log_test_report_pass();
    }

    void tc2() {
        mp3_encoder enc;
        std::error_code err;

        fs::path file_path { };
        err = enc.encode_wav(file_path);
        assert(err == std::errc::no_such_file_or_directory);

        /* TODO fs::path file_path1 {fs::current_path()};
         err = enc.encode_wav(file_path1);
         assert(err == std::errc::no_such_file_or_directory);*/

        fs::path file_path2 { "/asd/" };
        err = enc.encode_wav(file_path2);
        assert(err == std::errc::no_such_file_or_directory);

        err = enc.encode_wav(test_file_path1);
        assert(err == std::error_code { }); // sucsess
        log_test_report_pass();
    }

    static int tc3() {
        mp3_encoder enc;


        auto file1_path = test_data_dir_path / fs::path("testcase.wav");
        auto err = enc.encode_wav(file1_path);
        assert(err == std::error_code { });
        assert(fs::exists(test_data_dir_path / "testcase.mp3"));

        auto dir_path2 = test_data_dir_path / fs::path("testcase1.wav");
        err = enc.encode_wav(dir_path2);
        assert(fs::exists(test_data_dir_path / "testcase1.mp3"));
        log_test_report_pass();
        return 0;
    }

    static int tc4() {
        mp3_encoder enc;

        auto file1_path = test_data_dir_path / fs::path("testcase.wav");

        auto err = enc.encode_wav(file1_path, false);
        assert(err == std::error_code { });
        assert(fs::exists(test_data_dir_path / "testcase.mp3"));

        auto dir_path2 = test_data_dir_path / fs::path("testcase1.wav");
        err = enc.encode_wav(dir_path2, false);
        assert(fs::exists(test_data_dir_path / "testcase1.mp3"));
        log_test_report_pass();

        return 0;
    }
};

class test_wait_notify_que {

public:

    static void test() {
        test_wait_notify_que t1;
        t1.tc0();
        t1.tc1();
    }
private:
    wait_notify_queue<int> q1;
    void function1() {
        int i = 0;
        for (;;) {
            int x = q1.pop();
            if (x == -1)
                break;
            if (x != i)
                break;
            i++;
        }
        /*if(i == 10){
         std::cout<<"WaitNotifyQueTest passed" <<std::endl;
         }*/
    }
    void tc0() {
        std::thread t1(&test_wait_notify_que::function1, this);

        for (int i = 0; i < 10; i++)
            q1.push(i);

        q1.push(-1);
        t1.join();
        log_test_report_pass();
    }
    void tc1() {
        std::thread t1(&test_wait_notify_que::function1, this);
        std::thread t2(&test_wait_notify_que::function1, this);

        for (int i = 0; i < 10; i++) {
            q1.push(i);
            q1.push(i);
        }

        q1.push(-1);
        q1.push(-1);
        t1.join();
        t2.join();
        log_test_report_pass();
    }

};

class test_concurrent_thread {
public:
    static void test() {
        test_concurrent_thread tct;
        tct.tc0();
        tct.tc1();
        tct.tc3();
        tct.tc4();
    }
private:
    void tc0() {
        concurrent_encoder conc_enc;
        encoder_param_config config;
        auto num_th = conc_enc.create_threads(config);
        assert(num_th == 3);

        int num_core = std::thread::hardware_concurrency();

        auto num_th1 = conc_enc.create_threads(config, 100);
        assert(num_th1 == (num_core - num_th));
        log_test_report_pass();

    }
    void tc1() {
        fs::path dir_path { fs::current_path() };
        concurrent_encoder conc_enc;
        encoder_param_config config;

        auto num_th = conc_enc.create_threads(config, 4);
        assert(num_th == 4);

        auto num_th_join = conc_enc.join_all();
        assert(num_th_join == 4);

        num_th_join = conc_enc.join_all();
        assert(num_th_join == 0);
        log_test_report_pass();
    }
    void tc3() {
        concurrent_encoder conc_enc;
        try {
            fs::path file_path { };
            conc_enc.encode_wav_files(file_path);
        } catch (std::system_error& err) {
            if (LOG_VERBOSE)
                std::cout << err.what() << std::endl;
            assert(err.code() == std::errc::no_such_file_or_directory);
        }

        try {
            fs::path file_path1 { test_data_dir_path };
            conc_enc.encode_wav_files(file_path1);
        } catch (std::system_error& err) {
            if (LOG_VERBOSE)
                std::cout << err.what() << std::endl;
            assert(err.code() == std::error_code { }); // sucsess
        }

        try {
            fs::path file_path2 { "/asd/" };
            conc_enc.encode_wav_files(file_path2);
        } catch (std::system_error& err) {
            if (LOG_VERBOSE)
                std::cout << err.what() << std::endl;
            assert(err.code() == std::errc::no_such_file_or_directory);
        }

        try {
            conc_enc.encode_wav_files(test_file_path1);
        } catch (std::system_error& err) {
            if (LOG_VERBOSE)
                std::cout << err.what() << std::endl;
            //TODO assert(err.code() == std::errc::no_such_file_or_directory);
        }

        log_test_report_pass();

    }
    void tc4() {
        fs::path dir_path { test_data_dir_path };
        concurrent_encoder conc_enc;
        encoder_param_config config;

        conc_enc.create_threads(config,3);
        conc_enc.encode_wav_files(dir_path);
        conc_enc.join_all();

        directory_reader dr1(dir_path);
        directory_reader dr2(dir_path, ".mp3");

        assert(dr1.get_num_file() == dr2.get_num_file());

        while (1) {
            auto wav_file_path = dr1.get_a_file();
            if (wav_file_path.empty())
                break;
            std::string mp3_file_name = "test_data/" + wav_file_path.stem().string() + ".mp3";
            if (LOG_VERBOSE)
                std::cout << " matching" << wav_file_path.filename() << " " << mp3_file_name
                        << "\n";
            assert(fs::exists(mp3_file_name));

        }
        log_test_report_pass();

    }

};

class test_encoder_cli{
    static int count_token(char *iptr, char delim) {
            int token_count = 0;
            while (*iptr && isspace(*iptr))
                iptr++;
            while (*iptr) {
                if ((*iptr != delim)) {
                    token_count++;
                    while (*iptr && (*iptr != delim))
                        iptr++;
                }
                else {
                    iptr++;
                }
            }
            return token_count;
        }

        static char** split_str(char* input, int* argc){
            char**  argv;
            int token_count = count_token(input, ' ');
            argv = (char**)malloc(sizeof(char*)*token_count);

            int i = 0;
            char *token = strtok(input, " ");
            while(token) {
                //puts(token);
                argv[i] = strdup(token);
                token = strtok(NULL, " ");
                i++;
            }
            assert(i == token_count);
            *argc = token_count;
            return argv;
        }
        static void tc0(){
            char str[] = "./encode_test /dir/to/home";
            int argc;
            char** argv = split_str(&str[0], &argc);
            encoder_cli cli;
            cli.main(argc, argv);
            log_test_report_pass();
        }
        static void tc1(){
            char str[] = "./encode_test data_dir";
            int argc;
            char** argv = split_str(&str[0], &argc);
            encoder_cli cli;
            cli.main(argc, argv);
            log_test_report_pass();
        }
public:
    static void test(){
       tc0();
       tc1();
    }

};

class encoder_test{
public:
    static void test(){
        if(TEST_REPORT_LOG)
            std::cout << " Running Test for encoder \n";
        test_directory_reader::test();
        test_wave_header::test();
        test_mp3_encoder::test();
        test_wait_notify_que::test();
        test_concurrent_thread::test();
        test_encoder_cli::test();
    }
};

int main(int argc, char** argv) {
    encoder_test::test();
    return 0;
}


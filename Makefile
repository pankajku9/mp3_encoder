
all: cli test


cli:
	g++ -std=c++17 -ggdb3 -Iext/include/lame  -o encode encoder.cpp encoder_cli.cpp  ext/lib/libmp3lame.a -lstdc++fs -lpthread

test:
	g++ -std=c++17 -ggdb3 -Iext/include/lame  -o encode_test encoder.cpp encoder_test.cpp ext/lib/libmp3lame.a -lstdc++fs -lpthread
	
cli_dy:
	g++ -std=c++17 -ggdb3 -Iext/include/lame -Lext/lib/ -o encode encoder.cpp encoder_cli.cpp -lmp3lame  -lstdc++fs -lpthread
	
test_dy:
	g++ -std=c++17 -ggdb3 -Iext/include/lame -Lext/lib/ -o encode_test encoder.cpp encoder_test.cpp -lmp3lame  -lstdc++fs -lpthread


clean:	
	rm  test_data/*mp3
	rm  encode
	rm  encode_test		
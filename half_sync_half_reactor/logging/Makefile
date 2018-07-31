run: logging_unittest.o date.o logging.o logstream.o timestamp.o timezone.o
	clang++ -std=c++11 -pthread -g logging_unittest.o date.o logging.o logstream.o timestamp.o timezone.o -o logging_unittest.out

logging_unittest.o : logging_unittest.cpp
	clang++ -std=c++11 -g -c logging_unittest.cpp

date.o : date.cpp
	clang++ -std=c++11 -g -c date.cpp

logging.o : logging.cpp
	clang++ -std=c++11 -g -c logging.cpp

logstream.o : logstream.cpp
	clang++ -std=c++11 -g -c logstream.cpp

timestamp.o : timestamp.cpp
	clang++ -std=c++11 -g -c timestamp.cpp

timezone.o : timezone.cpp
	clang++ -std=c++11 -g -c timezone.cpp

clean: 
	rm *.o
	rm *out

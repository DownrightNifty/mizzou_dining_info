# produces MizzouDining.o
c++ -c -std=c++11 -I/usr/local/opt/libxml2/include -I/usr/local/opt/openssl@1.1/include MizzouDining.cpp

# produces ./a.out executable
c++ -std=c++11 -Wl,-L/usr/local/opt/libxml2/lib,-lxml2,-L/usr/local/opt/openssl@1.1/lib,-lssl,-lcrypto MizzouDining.o httplib.o

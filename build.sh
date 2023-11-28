set -e

brew install openssl@1.1

brew unlink openssl@3

# add -v for verbose output (useful for debugging)
c++ -std=c++11 -I/usr/local/opt/libxml2/include -I/usr/local/opt/openssl@1.1/include -Wl,-L/usr/local/opt/libxml2/lib,-lxml2,-L/usr/local/opt/openssl@1.1/lib,-lssl,-lcrypto MizzouDining.cpp httplib.cc

brew link openssl@3

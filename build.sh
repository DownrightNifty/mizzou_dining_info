# add -v for verbose output (useful for debugging)
c++ -std=c++11 -stdlib=libc++ -I/usr/local/opt/libxml2/include -I/usr/local/opt/openssl@1.1/include -Wl,-Z,-L/usr/local/opt/libxml2/lib,-lxml2,-L/usr/local/opt/openssl@1.1/lib,-lssl,-lcrypto,-L/usr/lib,-L/usr/local/lib,-F/Library/Frameworks,-F/System/Library/Frameworks MizzouDining.cpp httplib.cc

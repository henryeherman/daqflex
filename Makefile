#Add your libusb installation path here:
LIBUSB_PATH = /path/to/libusb-built/

daq : main.o mccdevice.o pollingthread.o stringutil.o
	g++ -g -pthread -I$(LIBUSB_PATH)/include/libusb-1.0  -L$(LIBUSB_PATH)/lib -lusb-1.0 -o daq main.o mccdevice.o pollingthread.o stringutil.o
	
main.o: main.cpp main.h mccdevice.h datatypesandstatics.h databuffer.h
	g++ -g -I$(LIBUSB_PATH)/include/libusb-1.0 -c main.cpp
	
mccdevice.o: mccdevice.cpp mccdevice.h pollingthread.h datatypesandstatics.h databuffer.h stringutil.h
	g++ -g -I$(LIBUSB_PATH)/include/libusb-1.0 -c mccdevice.cpp
	
pollingthread.o: pollingthread.cpp pollingthread.h mccdevice.h datatypesandstatics.h databuffer.h
	g++ -g -I$(LIBUSB_PATH)/include/libusb-1.0 -c pollingthread.cpp
	
stringutil.o: stringutil.cpp stringutil.h
	g++ -g -I/local/ehanson/build-dep/libusb/include/libusb-1.0 -c stringutil.cpp
	
clean:
	rm daq *.o

daq : main.o mccdevice.o pollingthread.o stringutil.o
	g++ -lpthread -g -L /local/b225404/apps/libusb-built/lib -lusb-1.0 -o daq main.o mccdevice.o pollingthread.o stringutil.o
	
main.o: main.cpp main.h mccdevice.h datatypesandstatics.h databuffer.h
	g++ -c -g -I /local/b225404/apps/libusb-built/include/libusb-1.0 main.cpp
	
mccdevice.o: mccdevice.cpp mccdevice.h pollingthread.h datatypesandstatics.h databuffer.h stringutil.h
	g++ -c -g -I /local/b225404/apps/libusb-built/include/libusb-1.0 mccdevice.cpp
	
pollingthread.o: pollingthread.cpp pollingthread.h mccdevice.h datatypesandstatics.h databuffer.h
	g++ -c -g -I /local/b225404/apps/libusb-built/include/libusb-1.0 pollingthread.cpp
	
stringutil.o: stringutil.cpp stringutil.h
	g++ -c -g stringutil.cpp
	
clean:
	rm daq *.o

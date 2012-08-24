/* Demonstrates using a thread to capture scan data in a circular buffer.
    This allows for running a scan in the background.
*/

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <libusb.h>

#include "mccdevice.h"
#include "datatypesandstatics.h"
#include "databuffer.h"
//#include "libusb.h"

//Constants for this device
#define DEVICE USB_1608_FS_PLUS

#define FILENAME "/local/ehanson/testfile.csv"

//Class declarations
template<class T> T fromString(const std::string& s);
void displayAndWriteData(unsigned short* data, int transferred, int numChans, ofstream* outputFile);
void fillCalConstants(unsigned int lowChan, unsigned int highChan);
int kbhit (void);

//Values used for calibration
float *calSlope;
float *calOffset;
int minVoltage;
int maxVoltage;

using namespace std;

MCCDevice* device;

int main(int argc, char *argv[])
{
    // TODO: Why can't do only 1 channel?
    unsigned int lowChan = 0;
    unsigned int highChan = 1;
    unsigned int numChans = highChan-lowChan+1;
    int rate = 22050;

    //numSamples * numChans must be an
    //integer multiple of 32 for a continuous scan
    int numSamples = 320; //Half of the buffer will be handled at a time.
    dataBuffer* buffer;
    bool lastHalfRead = SECONDHALF;

    string response;
    stringstream strLowChan, strHighChan, strRate, strNumSamples,
                        strCalSlope, strCalOffset, strRange;

    calSlope = new float[numChans];
    calOffset = new float[numChans];

    //open the output file
    ofstream outputFile;
    outputFile.open(FILENAME);

    try{
        /*Initialize the device
          note that MCCDevice(int idProduct, string mfgSerialNumber) could be used
          to find a device given a unique manufacturer serial number.
        */
        device = new MCCDevice(DEVICE);

        //create a buffer for the scan data
        buffer = new dataBuffer(numSamples*numChans);

        //Flush out any old data from the buffer
        device->flushInputData();

        //Configure an input scan
        device->sendMessage("AISCAN:XFRMODE=BLOCKIO"); //Good for fast acquisitions
        //device->sendMessage("AISCAN:XFRMODE=SINGLEIO"); //Good for slow acquisitions

        device->sendMessage("AISCAN:RANGE=BIP10V");//Set the voltage range on the device
        minVoltage = -10;//Set range for scaling purposes
        maxVoltage = 10;

        strLowChan << "AISCAN:LOWCHAN=" << lowChan;//Form the message
        device->sendMessage(strLowChan.str());//Send the message

        strHighChan << "AISCAN:HIGHCHAN=" << highChan;
        device->sendMessage(strHighChan.str());

        strRate << "AISCAN:RATE=" << rate;
        device->sendMessage(strRate.str());

        device->sendMessage("AISCAN:SAMPLES=0");//Continuous scan, set samples to 0

        //Fill cal constants for later use
        fillCalConstants(lowChan, highChan);

        device->sendMessage("AISCAN:START");//Start the scan on the device

        //Start collecting data in the background
        //Data buffer info will be stored in the buffer object
        device->startContinuousTransfer(rate, buffer, numSamples);
    }
    catch(mcc_err err)
    {
        delete device;
        cout << errorString(err);
        return 1;
    }

    cout << "Press enter to exit\n";

    while(!kbhit())
    {
        //Main loop of program

        //Data is placed into a circular buffer
        //Make sure to check buffer often enough so that you do not lose data
        //Only half the buffer is read at a time to avoid collisions
        if((buffer->currIndex > buffer->getNumPoints()/2) && (lastHalfRead == SECONDHALF))
        {
            //cout << "First Half Ready\n";
            displayAndWriteData(buffer->data, buffer->getNumPoints()/2, numChans, &outputFile);
            lastHalfRead = FIRSTHALF;
        }
        else if ((buffer->currIndex < buffer->getNumPoints()/2) && (lastHalfRead == FIRSTHALF))
        {
            //cout << "Second Half Ready\n";
            displayAndWriteData(&buffer->data[buffer->getNumPoints()/2], buffer->getNumPoints()/2, numChans, &outputFile);
            lastHalfRead = SECONDHALF;
        }
    }

    cout << "Done\n";
    device->stopContinuousTransfer();
    device->sendMessage("AISCAN:STOP");
	
    
    //close the output file
    outputFile.close();

    //delete the device and buffer ensuring proper cleanup
    delete buffer;
    delete calSlope;
    delete calOffset;
    delete device;
    
    return 0;
}

void displayAndWriteData(unsigned short* data, int transferred, int numChans, ofstream* outputFile)
{
    int currentChan, j, currentData=0;
    float fixedData;
    int numToDisplay = 10;

    cout << "Data (" << transferred/numChans << " transferred, displaying first " << numToDisplay << "):\n";
    for(j=0; j < transferred/2; j++)
    {
        for(currentChan=0; currentChan < numChans; currentChan++)
        {
            //Scale the data
            fixedData = device->scaleAndCalibrateData(data[currentData], minVoltage, maxVoltage, calSlope[currentChan], calOffset[currentChan]);

            //if(j<numToDisplay)//Limit console output to a few points for a cleaner display
                //cout << fixedData << ",";
            //Output all data to a csv file
            *outputFile << fixedData << ",";
            currentData++;
        }
        *outputFile << "\n";
        //if(j<numToDisplay)
        //cout << "\n";
    }

    // cout << "\n";
}

//Cal constants are only valid for the current set range.
//If the range is changed, the cal constants will need
//to be updated by running this function again.
void fillCalConstants(unsigned int lowChan, unsigned int highChan)
{
    unsigned int currentChan;
    stringstream strCalSlope, strCalOffset;
    string response;

    for(currentChan=lowChan; currentChan <= highChan; currentChan++){
        //Clear out last channel message
        strCalSlope.str("");
        strCalOffset.str("");

        //Set up the string message
        strCalSlope<< "?AI{" << currentChan << "}:SLOPE";
        strCalOffset<< "?AI{" << currentChan << "}:OFFSET";

        //Send the message and put the values into an array containing all channel cal data
        response = device->sendMessage(strCalSlope.str());
        calSlope[currentChan] = fromString<float>(response.erase(0,12));
        response = device->sendMessage(strCalOffset.str());
        calOffset[currentChan] = fromString<float>(response.erase(0,13));

        cout << "Channel " << currentChan << " Calibration Slope: " << calSlope[currentChan] << " Offset: " << calOffset[currentChan] << "\n";
    }
}

//converts a stringstream to a numeric value
template<class T>
T fromString(const std::string& s)
{
    std::istringstream stream (s);
    T t;
    stream >> t;
    return t;
}

//Workaround for not having a kbhit
int kbhit(void)
{
  struct timeval tv;
  fd_set rdfs;

  tv.tv_sec = 0;
  tv.tv_usec = 0;

  FD_ZERO(&rdfs);
  FD_SET (STDIN_FILENO, &rdfs);

  select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
  return FD_ISSET(STDIN_FILENO, &rdfs);
}

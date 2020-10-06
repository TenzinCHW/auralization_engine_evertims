#ifndef DIRECTIVITYHANDLER_H_INCLUDED
#define DIRECTIVITYHANDLER_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#include <iostream>
#include <math.h>

#include <mysofa.h>

class DirectivityHandler
{
    
//=========================================================================
// ATTRIBUTES
    
public:
    
private:
    
    const static int FILTER_LENGTH = 10; // num frequency bands expected
    int filter_length; // num freq bands in file
    float sampleRate = 48000; // dummy, just made it fit .sofa file to avoid resampling
    
    struct MYSOFA_EASY *sofaEasyStruct;
	bool isLoaded = false; // to know if need is to free struct at exit
    
    // to comply with libmysofa notations
    float leftIR[FILTER_LENGTH]; // gain real
    float rightIR[FILTER_LENGTH]; // gain imag
    float leftDelay; // dummy
    float rightDelay; // dummy
    
    Array<float> dirGains; // [gainReal0 gainReal1 .. gainRealFILTER_LENGTH-1 gainImag0 gainImag1 .. gainImagFILTER_LENGTH-1]
    
//==========================================================================
// METHODS
    
public:
    
DirectivityHandler()
{
}
        
~DirectivityHandler()
{
    // free sofa structure
	if( isLoaded ){ mysofa_close(sofaEasyStruct); }
}
  
void loadFile( const string & filenameStr )
{
    // get file path
    File hrirFile = getFileFromString(filenameStr);
    
    // convert to fit libmysofa expected input format
    String path = hrirFile.getFullPathName();
    const char *filename = path.getCharPointer();
    
	// Windows: skip directivity for now (bug at load)
	if (SystemStats::getOperatingSystemName().startsWithIgnoreCase("Win")){ 
		isLoaded = false;
		return;
	}

    // load
    int err;
    filter_length = 0;
    sofaEasyStruct = mysofa_open_no_norm(filename, sampleRate, &filter_length, &err);
    
    // check if file loaded correctly
    jassert( sofaEasyStruct != NULL );
    
    // check if expected size matches actual
    jassert( filter_length == FILTER_LENGTH );
    
    // resize locals
    dirGains.resize( 2 * filter_length );

    // warn if error
    if(sofaEasyStruct==NULL)
    {
		isLoaded = false;
        AlertWindow::showMessageBoxAsync ( AlertWindow::WarningIcon, "failed to load file", filenameStr, "OK");
    }
	else{ isLoaded = true; }

    // print info
    // printGains( 8, 15 );
}

Array<float> getGains( const double azim, const double elev )
{
    // make sure values are in expected range
    jassert(azim >= -M_PI && azim <= M_PI);
    jassert(elev >= -M_PI/2 && elev <= M_PI/2);
    
    // sph to cart
    float x = cosf(elev)*cosf(azim);
    float y = cosf(elev)*sinf(azim);
    float z = sinf(elev);
    
    // get interpolated gain value
    mysofa_getfilter_float( sofaEasyStruct, x, y, z, leftIR, rightIR, &leftDelay, &rightDelay );
    
    // fill output
    for( int i = 0; i < FILTER_LENGTH; i++ )
    {
        dirGains.set(i, (float)( leftIR[i] ));
        dirGains.set(FILTER_LENGTH + i, (float)( rightIR[i] ));
    }
    
    return dirGains;
}
    
void printGains(const unsigned int bandId, const unsigned int step )
{
    // query
	float leftIR[FILTER_LENGTH];
	float rightIR[FILTER_LENGTH];
    float leftDelay;          // unit is samples
    float rightDelay;         // unit is samples

    float azim = 0;
    float elev = 0;

    float x;
    float y;
    float z;

    for( int el = -90; el <= 90; el+=step )
    {
        for( int az = 0; az < 360; az+=step )
        {
            azim = az * (M_PI / 180.f);
            elev = el * (M_PI / 180.f);

            x = cosf(elev)*cosf(azim);
            y = cosf(elev)*sinf(azim);
            z = sinf(elev);

            mysofa_getfilter_float( sofaEasyStruct, x, y, z, leftIR, rightIR, &leftDelay, &rightDelay );
            
            std::cout << az << " " << el << " " << 1 << " " << leftIR[bandId] << " " << rightIR[bandId] << std::endl;
        }
    }
}
    
private:
    
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DirectivityHandler)
    
};

#endif // BINAURALENCODER_H_INCLUDED

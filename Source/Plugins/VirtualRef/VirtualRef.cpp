/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2014 Open Ephys

------------------------------------------------------------------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include "VirtualRef.h"
#include "VirtualRefEditor.h"


#define MIN(a,b) (((a)<(b))?(a):(b))


VirtualRef::VirtualRef()
    : GenericProcessor("Virtual Ref"), channelBuffer(1, BUFFER_SIZE), 
avgBuffer(1, BUFFER_SIZE), globalGain(1.0f)
{
	int nChannels = getNumInputs();
	refMat = new ReferenceMatrix(nChannels);
}

VirtualRef::~VirtualRef()
{
	delete refMat;
}

AudioProcessorEditor* VirtualRef::createEditor()
{
    editor = new VirtualRefEditor(this, true);
    return editor;
}



void VirtualRef::updateSettings()
{
	int nChannels = getNumInputs();

	if (refMat == nullptr)
	{
		refMat = new ReferenceMatrix(nChannels);
	}
	else
	{
		refMat->setNumberOfChannels(nChannels);
	}

	if (editor != nullptr)
	{
		editor->updateSettings();
	}
}


void VirtualRef::setParameter(int parameterIndex, float newValue)
{

}

void VirtualRef::process(AudioSampleBuffer& buffer,
                             MidiBuffer& midiMessages)
{
	float* ref;
	float refGain;
	int numRefs;
	int numChan = refMat->getNumberOfChannels();

	channelBuffer = buffer;

	for (int i=0; i<numChan; i++)
	{
		avgBuffer.clear();

		ref = refMat->getChannel(i);
		numRefs = 0;
		for (int j=0; j<numChan; j++)
		{
			if (ref[j] > 0)
			{
				numRefs++;
			}
		}

		for (int j=0; j<numChan; j++)
		{
			if (ref[j] > 0)
			{
				refGain = 1.0f / float(numRefs);
				avgBuffer.addFrom(0, 0, channelBuffer, j, 0,
								  channelBuffer.getNumSamples(), refGain);
			}
		}
		

		if (numRefs > 0)
		{
			buffer.addFrom(i, 			// destChannel
						0, 				// destStartSample
						avgBuffer, 	// source
						0,  			// sourceChannel
						0, 				// sourceStartSample
						buffer.getNumSamples(), // numSamples
						-1.0f * globalGain);  // global gain to apply 
		}
	}
}

ReferenceMatrix* VirtualRef::getReferenceMatrix()
{
	return refMat;
}

void VirtualRef::setGlobalGain(float value)
{
	globalGain = value;
}

float VirtualRef::getGlobalGain()
{
	return globalGain;
}

void VirtualRef::saveCustomParametersToXml(XmlElement* xml)
{
	int numChannels = refMat->getNumberOfChannels();

    xml->setAttribute("Type", "VirtualRef");

    XmlElement* paramXml = xml->createNewChildElement("PARAMETERS");
    paramXml->setAttribute("GlobalGain", getGlobalGain());
	paramXml->setAttribute("NumChannels", numChannels);

	XmlElement* channelsXml = xml->createNewChildElement("REFERENCES");

    for (int i=0; i<numChannels; i++)
    {
		float* ref = refMat->getChannel(i);
 
        XmlElement* channelXml = channelsXml->createNewChildElement("CHANNEL");
        channelXml->setAttribute("Index", i+1);
		for (int j=0; j<numChannels; j++)
		{
			if (ref[j] > 0)
			{
				XmlElement* refXml = channelXml->createNewChildElement("REFERENCE");
				refXml->setAttribute("Index", j+1);
				refXml->setAttribute("Value", ref[j]);
			}
		}
    }
}

void VirtualRef::loadCustomParametersFromXml()
{
	forEachXmlChildElementWithTagName(*parametersAsXml,	paramXml, "PARAMETERS")
	{
    	float globGain = (float)paramXml->getDoubleAttribute("GlobalGain");
		setGlobalGain(globGain);
	}

	forEachXmlChildElementWithTagName(*parametersAsXml,	channelsXml, "REFERENCES")
	{
		forEachXmlChildElementWithTagName(*channelsXml,	channelXml, "CHANNEL")
		{
			int channelIndex = channelXml->getIntAttribute("Index");

			forEachXmlChildElementWithTagName(*channelXml,	refXml, "REFERENCE")
			{
				int refIndex = refXml->getIntAttribute("Index");
				float gain = (float)refXml->getDoubleAttribute("Value");
				refMat->setValue(channelIndex - 1, refIndex - 1, gain);
			}
		}
	}

	updateSettings();
}


/* -----------------------------------------------------------------
ReferenceMatrix
----------------------------------------------------------------- */

ReferenceMatrix::ReferenceMatrix(int nChan)
{
	nChannels = nChan;
	nChannelsBefore = -1;
	values = nullptr;
	update();
}

ReferenceMatrix::~ReferenceMatrix()
{
	if (values != nullptr)
		delete[] values;
}

void ReferenceMatrix::setNumberOfChannels(int n)
{
	nChannels = n;
	update();
}

int ReferenceMatrix::getNumberOfChannels()
{
	return nChannels;
}

void ReferenceMatrix::update()
{
	if (nChannels != nChannelsBefore)
	{
		if (values != nullptr)
			delete[] values;

		values = new float[nChannels * nChannels];
		for (int i=0; i<nChannels * nChannels; i++)
			values[i] = 0;

		nChannelsBefore = nChannels;
	}
}

void ReferenceMatrix::setValue(int rowIndex, int colIndex, float value)
{
	if (rowIndex >= 0 && rowIndex < nChannels && colIndex >= 0 && colIndex < nChannels)
	{
		values[rowIndex * nChannels + colIndex] = value;
	}
	else
	{
		std::cout << "RefMatrix::setValue INDEX OUT OF BOUNDS! (rowIndex=" << rowIndex << ", colIndex=" << colIndex << ")" << std::endl;
	}
}

float ReferenceMatrix::getValue(int rowIndex, int colIndex)
{
	float value = -1;
	if (rowIndex >= 0 && rowIndex < nChannels && colIndex >= 0 && colIndex < nChannels)
	{
		value = values[rowIndex * nChannels + colIndex];
	}

	return value;
}

float* ReferenceMatrix::getChannel(int index)
{
	if (index >= 0 && index < nChannels)
		return &values[index * nChannels];
	else
		return nullptr;
}

bool ReferenceMatrix::allChannelReferencesActive(int index)
{
	float* chan = getChannel(index);

	int nActive = 0;

	if (chan != nullptr)
	{
		for (int i=0; i<nChannels; i++)
		{
			if (chan[i] > 0)
			{
				nActive++;
			}
		}
	}

	return nActive == nChannels;
}

void ReferenceMatrix::setAll(float value)
{
	if (values != nullptr)
	{
		for (int i=0; i<nChannels; i++)
		{
			for (int j=0; j<nChannels; j++)
			{
				values[i*nChannels + j] = value;
			}
		}
	}
}

void ReferenceMatrix::setAll(float value, int maxChan)
{
	if (values != nullptr)
	{
		maxChan = MIN(nChannels, maxChan);
		for (int i=0; i<maxChan; i++)
		{
			for (int j=0; j<maxChan; j++)
			{
				values[i*nChannels + j] = value;
			}
		}
	}
}

void ReferenceMatrix::clear()
{
	if (values != nullptr)
	{
		for (int i=0; i<nChannels; i++)
		{
			for (int j=0; j<nChannels; j++)
			{
				values[i*nChannels + j] = 0;
			}
		}
	}
}

void ReferenceMatrix::print()
{
	for (int i=0; i<nChannels; i++)
	{
		float* chan = getChannel(i);
		for (int j=0; j<nChannels; j++)
		{
			std::cout << chan[j] << " ";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}


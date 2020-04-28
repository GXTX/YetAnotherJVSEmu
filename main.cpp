#include <iostream>
#include <thread>

#include "Includes/JvsIo.h"
#include "Includes/SerIo.h"
#include "Includes/GpIo.h"
#include "Includes/SdlIo.h"

#ifdef WII
#include "Includes/WiiIo.h"
#endif

#include <sched.h>

int main()
{
	// Set thread priority to RT. We don't care if this
	// fails but may be required for some systems.
	struct sched_param params;
	params.sched_priority = sched_get_priority_max(SCHED_FIFO);
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &params);

	// TODO: Just use a std::string or something..
	char serialName[13];
	std::sprintf(serialName, "/dev/ttyUSB0");

	// TODO: maybe put set these as shared in serio?
	std::vector<uint8_t> ReadBuffer;
	std::vector<uint8_t> WriteBuffer;

	std::unique_ptr<GpIo> GPIOHandler (std::make_unique<GpIo>(GpIo::SenseType::Float));
	if (!GPIOHandler->IsInitialized) {
		std::cerr << "Couldn't initiate GPIO and \"NONE\" wasn't explicitly set." << std::endl;
		return 1;
	}

	// TODO: probably doesn't need to be shared? we only need Inputs to be a shared ptr
	std::shared_ptr<JvsIo> JVSHandler (std::make_shared<JvsIo>(JvsIo::SenseStates::NotConnected));

	std::unique_ptr<SerIo> SerialHandler (std::make_unique<SerIo>(serialName));
	if (!SerialHandler->IsInitialized) {
		std::cerr << "Coudln't initiate the serial controller." << std::endl;
		return 1;
	}

	// Spawn lone SDL2 or WiiIo input thread.
	// NOTE: There probably is no reason we can't have both of these running
	// at once but we want to avoid a situation where SDL attempts to attach
	// the wiimote to it's thread. There are other ways this can be
	// avoided but this will work for now.
#ifdef WII
	std::thread(&WiiIo::Loop, std::make_unique<WiiIo>(&JVSHandler->Inputs)).detach();
#else
	std::thread(&SdlIo::Loop, std::make_unique<SdlIo>(&JVSHandler->Inputs)).detach();
#endif

	while (true) {
		ReadBuffer.resize(512);
		SerialHandler->Read(ReadBuffer.data());

		if (ReadBuffer.size() > 0) {
			int temp = JVSHandler->ReceivePacket(ReadBuffer.data());
			// TODO: Why without this does it fault?
			WriteBuffer.resize(ReadBuffer.size());
			ReadBuffer.clear();

			int ret = JVSHandler->SendPacket(WriteBuffer.data());

			if (ret > 0) {
				if(JVSHandler->pSenseChange){
					if(JVSHandler->pSense == JvsIo::SenseStates::NotConnected) {
						GPIOHandler->SetMode(GpIo::PinMode::In);
					}
					else {
						GPIOHandler->SetMode(GpIo::PinMode::Out);
						GPIOHandler->Write(GpIo::PinState::Low);
					}
					JVSHandler->pSenseChange = false;
				}
				SerialHandler->Write(WriteBuffer.data(), ret);
				WriteBuffer.clear();
			}
		}
	}

	return 0;
}

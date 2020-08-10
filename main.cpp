/* CHECKLIST:
-LISTA POŁĄCZEŃ TWORZONA ZA POMOCĄ PADA,
-OTWIERANIE PORTÓW POSZCZEGÓLNYCH URZĄDZEŃ MIMO ZAPOBIEGAJĄC PRZYPADKOWI ICH PRZEŁĄCZENIA,
-Obniżanie oktawy,
-Błąd ucinanych note off-ów ???
*/
#include <iostream>
#include <signal.h>
#include "RtMidi.h"
#include <unistd.h>
#include "midi.h"
#include "string"
#include "thread"
#define DEVICE_COUNT 4
#define NOTE_ON 0x90
#define NOTE_OFF 0x90


namespace
{
    RtMidiIn *midiin = new RtMidiIn(); //wejście z pada
    std::vector<unsigned char> messagein;
    RtMidiIn *midiin2 = new RtMidiIn(); //RtMidi::UNSPECIFIED,"RTMidi Input Client2", (unsigned int)100
    std::vector<unsigned char> messagein2;
    RtMidiIn *midiin3 = new RtMidiIn(); //RtMidi::UNSPECIFIED,"RTMidi Input Client1", (unsigned int)100
    std::vector<unsigned char> messagein3;
    RtMidiIn *midiin4 = new RtMidiIn(); //RtMidi::UNSPECIFIED,"RTMidi Input Client2", (unsigned int)100
    std::vector<unsigned char> messagein4;

    RtMidiOut *midiout = new RtMidiOut(); //wyjście do pada
    std::vector<unsigned char> messageout;
    RtMidiOut *midiout2 = new RtMidiOut(); //wyjście miditech out3
    std::vector<unsigned char> messageout2;
    RtMidiOut *midiout3 = new RtMidiOut(); //wyjście do pianina
    std::vector<unsigned char> messageout3;

    unsigned char ucElementInList[DEVICE_COUNT];
    bool bList[DEVICE_COUNT][13];
    unsigned char ucActiveNotes[DEVICE_COUNT];

    bool bFeedback0 = 0;
    bool bFeedback1 = 0;
    bool bFeedback2 = 0;

    enum State {OFF, ON} ;
    enum Direction {DOWN, UP} ;

    bool done = false;
    static void finish(int ignore){ done = true; }
    unsigned char ucChannel;
    unsigned char urzadzenie = 0;
    unsigned int nBytes, nBytes2, nBytes3, nBytes4;
}

unsigned char ucGetType(unsigned char message)
{
    message = message & 0xF0;
    return message;
}

void SendNoteOffToDevice(unsigned char DeviceNumber, unsigned char ucChannelNumber)
{
    std::vector<unsigned char> message;
    message.resize(3);
    message[0] = ucChnChange(0x90, ucChannelNumber);
    message[2] = 0;
    for(unsigned char i=21; i<109; i++)
    {
        message[1] = i;
        midiout2->sendMessage(&message);
        usleep(10);
    }
}

void PadLedInit()
{
    messageout[0]=NOTE_ON;
    messageout[2]=0;
    for(unsigned char ucNotes=12; ucNotes<117; ucNotes++)
    {
        messageout[1]=ucNotes;
        midiout->sendMessage(&messageout);
        usleep(10);
    }
}

unsigned char ucChangeNoteOctave(unsigned char byte_1, enum Direction eDownUp, unsigned char ucCount)
{
    if(UP==eDownUp)
    {
        byte_1 += 12*ucCount;
    }
    else
    {
        byte_1 -= 12*ucCount;
    }
    return byte_1;
}

void ChangeChannelState(bool bLista[][13], unsigned char ucNote ,unsigned char ucChannel, unsigned char ucDeviceNumber)
{
    if(ucChannel!=0)
    {
        messageout[0] = NOTE_ON;
        messageout[1] = ucNote;
        if(ON==bLista[ucDeviceNumber][ucChannel])
        {
            messageout[2]=0;
            bLista[ucDeviceNumber][ucChannel] = 0;
            ucElementInList[ucDeviceNumber] -= 1;
            if(ucActiveNotes[ucDeviceNumber]!=0) //obsługa błędu pozostającej nuty po wyłączeniu kanału
            {
                SendNoteOffToDevice(ucDeviceNumber, ucChannel); //funkcja wysyła noteoffy
            }
        }
        else
        {
            messageout[2]=127;
            bLista[ucDeviceNumber][ucChannel] = 1;
            ucElementInList[ucDeviceNumber] += 1;
        }
        std::cout << (int)messageout[1] << std::endl;
        midiout->sendMessage(&messageout);
    }
}


void SendToSelectedChn(unsigned char ucDeviceNumber, std::vector<unsigned char> * vucMessage)
{
    unsigned char temp = 0;
    unsigned char temp_2 = 0;
    for(unsigned char i = 0; temp<ucElementInList[ucDeviceNumber]; i++) //i<12
    {
        if(ON==bList[ucDeviceNumber][i+1])
        {
            temp_2 = i + 1;
            messageout2[0] = ucChnChange((*vucMessage)[0], temp_2);
            messageout2[1] = (*vucMessage)[1];
            messageout2[2] = (*vucMessage)[2];
            midiout2->sendMessage(&messageout2);
            temp++;
        }
        //usleep(200);
    }
}

void ActiveNotesCountChange(unsigned char ucDeviceNumber, std::vector<unsigned char> * message)
{
    unsigned char byte_0 = (*message)[0];
    byte_0 = byte_0 & 0xF0;
    if(byte_0 == 0x90 && (*message)[2] != 0)
    {
        ucActiveNotes[ucDeviceNumber] += 1;
    }
    else if( (byte_0 == 0x80) || (byte_0 == 0x90 && (*message)[2] == 0))
    {
        ucActiveNotes[ucDeviceNumber] -= 1;
    }
    //std::cout << (int)ucActiveNotes[ucDeviceNumber] << std::endl; //podgląd liczby aktualnie wciśniętych na urządzeniu przycisków
}

void thread_two()
{
    unsigned int g;
    while ( !done )
    {
        midiin2->getMessage( &messagein2 ); //pobieranie nut od portów do wektorów
        nBytes2 = messagein2.size(); //ilość bitów wiadomości
        midiin3->getMessage( &messagein3 );
        nBytes3 = messagein3.size();
        midiin4->getMessage( &messagein4 );
        nBytes4 = messagein4.size();
        for ( g=0; g<nBytes2; g++ )
        {
            /*if(g==0 || g==1)
            {
                std::cout << "Keyboard: " <<"Byte " << g << " = " << (int)messagein2[g] << ", ";
            }
            else
            {
                std::cout << "Byte " << g << " = " << (int)messagein2[g] << ", " << std::endl;
            }*/
        }
        if((nBytes2 > 0) && (3==ucGetChannel(messagein2[0]))) //zarządzanie nutami odebranymi od klawiatury górnej
        {
            //messagein2[1] = ucChangeNoteOctave(messagein2[1], DOWN, 1);
            SendToSelectedChn(0, &messagein2);
            ActiveNotesCountChange(0, &messagein2);
            usleep(30);
            //wysyłanie z powrotem do pianina
            if(1==bFeedback0)
            {
                messageout3[0] = ucChnChange(messagein2[0], 4);
                messageout3[1] = messagein2[1];
                messageout3[2] = messagein2[2];
                midiout3->sendMessage(&messageout3);
                std::cout << "ok" << std::endl;
            }
            usleep(30);
        }
        else if((nBytes2 > 0) && (2==ucGetChannel(messagein2[0]))) //zarządzanie nutami odebranymi od klawiatury dolnej
        {
            SendToSelectedChn(1, &messagein2);
            ActiveNotesCountChange(1, &messagein2);
            usleep(30);
            //wysyłanie z powrotem do pianina
            if(1==bFeedback1)
            {
                messageout3[0] = ucChnChange(messagein2[0], 4);
                messageout3[1] = messagein2[1];
                messageout3[2] = messagein2[2];
                midiout3->sendMessage(&messageout3);
                std::cout << "ok" << std::endl;
            }
            usleep(30);
        }
        for ( g=0; g<nBytes3; g++ ) //podgląd nut wysyłanych przez pianino
        {
            /*if(g==0 || g==1)
            {
                std::cout << "Virtual Keyboard: " <<"Byte " << g << " = " << (int)messagein3[g] << ", ";
            }
            else
            {
                std::cout << "Byte " << g << " = " << (int)messagein3[g] << ", " << std::endl;
            }*/

        }
        if(nBytes3 > 0) //zarządzanie nutami odebranymi przez pedały
        {
            //wysyłanie z powrotem do pianina z obniżeniem oktawy
            if(1==bFeedback2)
            {
                messageout3[0] = ucChnChange(messagein3[0], 4);
                messageout3[1] = ucChangeNoteOctave(messagein3[1], DOWN, 1);
                messageout3[2] = messagein3[2];
                midiout3->sendMessage(&messageout3);
                std::cout << "ok" << std::endl;
            }
            usleep(30);
            //messagein3[1] = ucChangeNoteOctave(messagein3[1], DOWN, 1);
            SendToSelectedChn(3, &messagein3);
            ActiveNotesCountChange(3, &messagein3);
            usleep(30);
        }
        for ( g=0; g<nBytes4; g++ ) //podgląd nut wysyłanych przez pianino
        {
            /*if(g==0 || g==1)
            {
                std::cout << "Piano: " <<"Byte " << g << " = " << (int)messagein4[g] << ", ";
            }
            else
            {
                std::cout << "Byte " << g << " = " << (int)messagein4[g] << ", " << std::endl;
            }
	    */
        }
        if(nBytes4 > 0) //zarządzanie nutami odebranymi od pedałów
        {
            SendToSelectedChn(2, &messagein4);
            ActiveNotesCountChange(2, &messagein4);
            usleep(30);
        }
    }
}

void thread_one()
{
    unsigned int i;
    while ( !done )
    {
        midiin->getMessage( &messagein );
        nBytes = messagein.size();
        /*for ( i=0; i<nBytes; i++ ) //podgląd wiadomości odbieranych od pada
        {
            if((messagein[i] & 0x80))
            {
                std::cout << "Pad: " << "Byte " << i << " = " << (int)messagein[i] << ", ";
            }
            else
            {
                std::cout << "Byte " << i << " = " << (int)messagein[i] << std::endl;
            }
        }*/
        if ( (nBytes > 0) && (messagein[2] != 0)) //
        {
            ucChannel = ucDecodeNoteToChn(messagein[1], &urzadzenie);
            if(0==ucChannel)
            {
                if(messagein[1]==43)
                {
                    messageout[0] = messagein[0];
                    messageout[1] = messagein[1];
                    messageout[2] = (bFeedback0==0) ? 127 : 0;
                    bFeedback0 ^= 1;
                    midiout->sendMessage(&messageout);
                }
                else if(messagein[1]==67)
                {
                    messageout[0] = messagein[0];
                    messageout[1] = messagein[1];
                    messageout[2] = (bFeedback1==0) ? 127 : 0;
                    bFeedback1 ^= 1;
                    midiout->sendMessage(&messageout);
                }
                else if(messagein[1]==91)
                {
                    messageout[0] = messagein[0];
                    messageout[1] = 43;
                    messageout[2] = (bFeedback0==1 || bFeedback1==1 || bFeedback2==1) ? 0 : 127;
                    if(messageout[2]==0)
                    {
                        bFeedback0=0;
                        bFeedback1=0;
                        bFeedback2=0;
                    }
                    else
                    {
                        bFeedback0=1;
                        bFeedback1=1;
                        bFeedback2=1;
                    }
                    midiout->sendMessage(&messageout);
                    usleep(30);
                    messageout[1] = 67;
                    midiout->sendMessage(&messageout);
                    usleep(30);
                    messageout[1] = 115;
                    midiout->sendMessage(&messageout);
                    usleep(30);
                    messageout[1] = 91;
                    messageout[2] = 127;
                    midiout->sendMessage(&messageout);
                    sleep(1);
                    messageout[2] = 0;
                    midiout->sendMessage(&messageout);
                    usleep(30);
                }
                else if(messagein[1]==115)
                {
                    messageout[0] = messagein[0];
                    messageout[1] = messagein[1];
                    messageout[2] = (bFeedback2==0) ? 127 : 0;
                    bFeedback2 ^= 1;
                    midiout->sendMessage(&messageout);
                }
            }
            else
            {
                ChangeChannelState(bList, messagein[1], ucChannel, urzadzenie);
            }
            //podgląd tablicy bList
            //char j, k; for(j=0; j<4; j++) { for(k=1; k<13; k++) { std::cout << bList[j][k] << " "; } std::cout << ucElementInList[k] << std::endl; }
        }
    }
}

void init()
{
    for(int i = 0; i<DEVICE_COUNT; i++) //inicjalizacja ucElemenInList
    {
        ucElementInList[i] = 0;
    }
    messageout.resize(3);
    messageout2.resize(3);
    messageout3.resize(3);
    (void) signal(SIGINT, finish);
    unsigned int i;
    // Check available ports.
    unsigned int nPortsIn = midiin->getPortCount();
    unsigned int nPortsOut = midiout->getPortCount();
    std::string str1 = "CMD Touch:CMD Touch MIDI 1";
    std::string out3 = "MIDI4x4:MIDI4x4 MIDI 3";
    std::string in1 = "MIDI4x4:MIDI4x4 MIDI 1";
    std::string in2 = "MIDI4x4:MIDI4x4 MIDI 2";
    std::string in3 = "MOTIF8:MOTIF8 MIDI 1"; //usb piano
    std::string out4 = "MOTIF8:MOTIF8 MIDI 1"; //usb piano
    if ( nPortsIn==0)
    {
        std::cout << "No ports available!\n";
    }
    for(i=0; i<nPortsIn; i++)
    {
        std::cout << midiin->getPortName(i) << std::endl;
        if((str1.compare(0, 12, midiin->getPortName(i).substr(0,12))==0))
        {
            midiin->openPort(i);
        }
        else if((in1.compare(0, 22, midiin2->getPortName(i).substr(0,22))==0))
        {
            midiin2->openPort(i);
        }
        else if((in2.compare(0, 22, midiin3->getPortName(i).substr(0,22))==0))
        {
            midiin3->openPort(i);
        }
        else if((in3.compare(0, 20, midiin4->getPortName(i).substr(0,20))==0))
        {
            midiin4->openPort(i);
        }
        std::cout << midiin4->getPortName(i).substr(0,20) << std::endl;
    }
    for(i=0; i<nPortsOut; i++)
    {
        if((str1.compare(0, 12, midiout->getPortName(i).substr(0,12))==0))
        {
            midiout->openPort(i);
        }
        else if((out3.compare(0, 22, midiout2->getPortName(i).substr(0,22))==0))
        {
            midiout2->openPort(i);
        }
        else if((out4.compare(0, 20, midiout3->getPortName(i).substr(0,20))==0))
        {
            midiout3->openPort(i);
        }
    }
    std::cout << "Czy port 1 in jest otwarty: " << (int)midiin->isPortOpen() << std::endl;
    std::cout << "Czy port 1 out jest otwarty: " << (int)midiout->isPortOpen() << std::endl;
    std::cout << "Czy port 2 in jest otwarty: " << (int)midiin2->isPortOpen() << std::endl;
    std::cout << "Czy port 2 out jest otwarty: " << (int)midiout2->isPortOpen() << std::endl;
    std::cout << "Czy port 3 in jest otwarty: " << (int)midiin3->isPortOpen() << std::endl;
    std::cout << "Czy port 4 in jest otwarty(pianino): " << (int)midiin4->isPortOpen() << std::endl;
    std::cout << "Czy port 4 out jest otwarty(pianino): " << (int)midiout3->isPortOpen() << std::endl;

    midiin->ignoreTypes( false, false, false ); // Don't ignore sysex, timing, or active sensing messages.
    midiin2->ignoreTypes( false, false, false );
    midiin3->ignoreTypes( false, false, false );
    midiin4->ignoreTypes( true, true, true );
}

int main()
{
    init();
    PadLedInit();
    std::cout << "App is ready.. Quit with Ctrl-C.\n";
    std::thread first (thread_two);
    std::thread second (thread_one);
    first.join();
    second.join();

    while(!done)
    {

    }
    first.~thread();
    second.~thread();
    // Clean up
    return 0;
}

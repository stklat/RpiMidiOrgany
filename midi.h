bool bCompareNote(unsigned char Note, unsigned char byte_1)
{
    if(Note==byte_1) { return true; }
    else { return false; }
}

unsigned char ucGetChannel(unsigned char byte_0)
{
    byte_0 = byte_0 & 0xF;
    byte_0++;
    return byte_0;
}

unsigned char ucChnChange(unsigned char byte_0, unsigned char ChnNumber)
{
    byte_0 = byte_0 & 0xFFF0;
    ChnNumber--;
    byte_0 |= ChnNumber;
    return byte_0;
}

unsigned char ucDecodeNoteToChn(unsigned char byte_1, unsigned char * ucDeviceNumber)
{
    *ucDeviceNumber = 0;
    if(23>=byte_1)
    {
        return 0;
    }
    else
    {
        byte_1 -= 23;
    }
    for(unsigned char i=0; i<10 ; i++)
    {
        if(byte_1<=8)
        {
            return byte_1;
        }
        else if((byte_1<=12) ||((byte_1>=17)&&(byte_1<=21)))
        {
            return 0;
        }
        else if(byte_1<=20)
        {
            byte_1 -= 4;
            return byte_1;
        }
        else
        {
            *ucDeviceNumber += 1;
            byte_1 -= 24;
        }
    }
}

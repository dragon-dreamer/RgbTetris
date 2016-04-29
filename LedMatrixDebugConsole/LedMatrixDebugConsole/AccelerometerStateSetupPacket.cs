// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0
namespace LedMatrixDebugConsole
{
    public class AccelerometerStateSetupPacket : IComPacketToSend
    {
        public bool IsAccelerometerOn { get; set; }

        public AccelerometerStateSetupPacket(bool isAccelerometerOn)
        {
            IsAccelerometerOn = isAccelerometerOn;
        }

        public byte[] GetRawData()
        {
            return new byte[] { 0xff, 0x04, IsAccelerometerOn ? (byte)0x01 : (byte)0x00 };
        }
    }
}

// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0
using System;

namespace LedMatrixDebugConsole
{
    abstract public class ReceivedComPacket
    {
    }

    public class ReceivedComPacketEventArgs : EventArgs
    {
        private readonly ReceivedComPacket _packet;

        public ReceivedComPacketEventArgs(ReceivedComPacket packet)
        {
            _packet = packet;
        }

        public ReceivedComPacket Packet
        {
            get { return _packet; }
        }
    }

    public delegate void OnReceivedComPacketHandler(object sender, ReceivedComPacketEventArgs e);

    public interface IReceivedComPacketType
    {
        event OnReceivedComPacketHandler Packet;

        int PacketLength { get; }
        bool CheckPacketSignature(byte signature);
        void ParsePacket(byte[] data);
    }
}

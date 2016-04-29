// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0
namespace LedMatrixDebugConsole
{
    public enum ButtonId
    {
        Up,
        Down,
        Left,
        Right,
        Fwd,
        Back
    }

    public class ButtonStatePacket : ReceivedComPacket
    {
        public ButtonId Id { get; set; }
        public bool IsPressed { get; set; }
    }

    public class ButtonStatePacketType : IReceivedComPacketType
    {
        public event OnReceivedComPacketHandler Packet;

        public int PacketLength { get { return 1; } }

        public bool CheckPacketSignature(byte signature)
        {
            return (signature & 0xF0) == 0xF0;
        }

        public void ParsePacket(byte[] data)
        {
            var handler = Packet;
            if (handler == null)
                return;

            ButtonId id;
            switch((data[0] & 0x0E) >> 1)
            {
                case 0:
                    id = ButtonId.Fwd;
                    break;

                case 1:
                    id = ButtonId.Back;
                    break;

                case 2:
                    id = ButtonId.Left;
                    break;

                case 3:
                    id = ButtonId.Right;
                    break;

                case 4:
                    id = ButtonId.Up;
                    break;

                case 5:
                    id = ButtonId.Down;
                    break;

                default:
                    return; //Unknown value
            }

            handler(this, new ReceivedComPacketEventArgs(
                new ButtonStatePacket { Id = id, IsPressed = (data[0] & 0x01) != 0 }));
        }
    }
}

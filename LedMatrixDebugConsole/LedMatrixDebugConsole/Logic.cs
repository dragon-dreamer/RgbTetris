// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0
using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Timers;
using System.Windows.Media;

namespace LedMatrixDebugConsole
{
    public class Logic : PropertyChangedHelper, IDisposable
    {
        private List<string> _comPortList = new List<string>();
        private string _selectedComPort = string.Empty;
        private ComPort _port;
        private Timer _timeoutTimer = new Timer(1000);

        private bool _canSendCommand = false;
        private bool _isConnectionOpen = false;
        private int _brightness = BrightnessSetupPacket.MinBrightness;
        private bool _isAccelerometerOn = false;
        private int _accelerometerX = 0, _accelerometerY = 0, _accelerometerZ = 0;
        private bool _fwdButtonPressed = false, _backButtonPressed = false;
        private bool _leftButtonPressed = false, _rightButtonPressed = false;
        private bool _upButtonPressed = false, _downButtonPressed = false;

        private ReadyPacketType _readyPacketType = new ReadyPacketType();
        private ButtonStatePacketType _buttonStatePacketType = new ButtonStatePacketType();
        private BrightnessPacketType _brightnessPacketType = new BrightnessPacketType();
        private AccelerometerStatePacketType _accelerometerStatePacketType = new AccelerometerStatePacketType();
        private AccelerometerValuesPacketType _accelerometerValuesPacketType = new AccelerometerValuesPacketType();

        public delegate void OnDeviceReadyHandler();
        public event OnDeviceReadyHandler DeviceReady;
        public delegate void OnButtonStateChangedHandler(ButtonId buttonId, bool isPressed);
        public event OnButtonStateChangedHandler ButtonStateChanged;
        public delegate void OnBrightnessReceivedHandler(int brightness);
        public event OnBrightnessReceivedHandler BrightnessReceived;
        public delegate void OnAccelerometerStateReceivedHandler(bool isAccelerometerOn);
        public event OnAccelerometerStateReceivedHandler AccelerometerStateReceived;
        public delegate void OnAccelerometerValuesReceivedHandler(Int16 x, Int16 y, Int16 z);
        public event OnAccelerometerValuesReceivedHandler AccelerometerValuesReceived;
        public delegate void OnEmptyPortListHandler();
        public event OnEmptyPortListHandler EmptyPortList;
        public delegate void OnComPortTimeoutHandler();
        public event OnComPortTimeoutHandler ComPortTimeout;

        public const int DeviceBaudRate = 115200;
        public const Parity DeviceParity = Parity.None;
        public const int DeviceDataBits = 8;
        public const StopBits DeviceStopBits = StopBits.One;

        public Logic()
        {
            _readyPacketType.Packet += OnReadyPacket;
            _buttonStatePacketType.Packet += OnButtonStatePacket;
            _brightnessPacketType.Packet += OnBrightnessPacket;
            _accelerometerStatePacketType.Packet += OnAccelerometerStatePacket;
            _accelerometerValuesPacketType.Packet += OnAccelerometerValuesPacket;
            _timeoutTimer.Elapsed += OnComPortTimeout;
            _timeoutTimer.AutoReset = false;
        }

        void OnComPortTimeout(object sender, ElapsedEventArgs e)
        {
            CloseConnection();
            var handler = ComPortTimeout;
            if (handler != null)
                handler();
        }

        public void RefreshPortList()
        {
            var list = SerialPort.GetPortNames().ToList();
            list.Sort();
            ComPortList = list;
            SelectedComPort = string.Empty;
            if (_comPortList.Count != 0)
            {
                SelectedComPort = _comPortList[0];
            }
            else
            {
                var handler = EmptyPortList;
                if (handler != null)
                    handler();
            }
        }

        public void OpenConnection()
        {
            CloseConnection();

            try
            {
                _port = new ComPort(_selectedComPort, DeviceBaudRate, DeviceParity, DeviceDataBits, DeviceStopBits);
                _port.AddPacketType(_readyPacketType);
                _port.AddPacketType(_buttonStatePacketType);
                _port.AddPacketType(_brightnessPacketType);
                _port.AddPacketType(_accelerometerStatePacketType);
                _port.AddPacketType(_accelerometerValuesPacketType);
                _port.Start();

                CanSendCommand = true;
                IsConnectionOpen = true;
            }
            catch(Exception)
            {
                CloseConnection();
                throw;
            }
        }

        public void CloseConnection()
        {
            if (_port != null)
            {
                _port.Dispose();
                _port = null;
                ResetProperties();
            }

            CanSendCommand = false;
            IsConnectionOpen = false;
        }

        public bool CanOpenConnection
        {
            get { return _selectedComPort != string.Empty && !_isConnectionOpen; }
        }

        public void SetNumberDisplayValue(UInt32 value)
        {
            WritePacket(new SetNumericDisplayValuePacket { Value = value });
        }

        public void RequestBrightness()
        {
            WritePacket(new BrightnessRequestPacket());
        }

        public void RequestAccelerometerState()
        {
            WritePacket(new AccelerometerStateRequestPacket());
        }

        public void RequestAccelerometerValues()
        {
            WritePacket(new AccelerometerValuesRequestPacket());
        }

        public void SetNumericDisplayValue(int value)
        {
            WritePacket(new SetNumericDisplayValuePacket { Value = (uint)value });
        }

        public void SendDisplayData(Color[,] data)
        {
            WritePacket(new DisplayPacket(data));
        }

        public void SendBrightnessPacket()
        {
            WritePacket(new BrightnessSetupPacket((byte)Brightness));
        }

        public void SendAccelerometerStatePacket()
        {
            WritePacket(new AccelerometerStateSetupPacket(IsAccelerometerOn));
        }

        public string SelectedComPort
        {
            get { return _selectedComPort; }
            set
            {
                if (_selectedComPort != value)
                {
                    _selectedComPort = value;
                    OnPropertyChanged();
                    OnPropertyChanged("CanOpenConnection");
                }
            }
        }

        public bool CanSendCommand
        {
            get { return _canSendCommand; }
            set
            {
                if (_canSendCommand != value)
                {
                    _canSendCommand = value;
                    OnPropertyChanged();
                }
            }
        }

        public bool IsConnectionOpen
        {
            get { return _isConnectionOpen; }
            set
            {
                if (_isConnectionOpen != value)
                {
                    _isConnectionOpen = value;
                    OnPropertyChanged();
                    OnPropertyChanged("CanOpenConnection");
                }
            }
        }

        public int Brightness
        {
            get { return _brightness; }
            set
            {
                if (_brightness != value)
                {
                    _brightness = value;
                    SendBrightnessPacket();
                    OnPropertyChanged();
                }
            }
        }

        public bool IsAccelerometerOn
        {
            get { return _isAccelerometerOn; }
            set
            {
                if (_isAccelerometerOn != value)
                {
                    _isAccelerometerOn = value;
                    SendAccelerometerStatePacket();
                    OnPropertyChanged();
                }
            }
        }

        public int AccelerometerX
        {
            get { return _accelerometerX; }
            set
            {
                if (_accelerometerX != value)
                {
                    _accelerometerX = value;
                    OnPropertyChanged();
                }
            }
        }

        public int AccelerometerY
        {
            get { return _accelerometerY; }
            set
            {
                if (_accelerometerY != value)
                {
                    _accelerometerY = value;
                    OnPropertyChanged();
                }
            }
        }

        public int AccelerometerZ
        {
            get { return _accelerometerZ; }
            set
            {
                if (_accelerometerZ != value)
                {
                    _accelerometerZ = value;
                    OnPropertyChanged();
                }
            }
        }

        public bool UpButtonPressed
        {
            get { return _upButtonPressed; }
            set
            {
                if (_upButtonPressed != value)
                {
                    _upButtonPressed = value;
                    OnPropertyChanged();
                }
            }
        }

        public bool DownButtonPressed
        {
            get { return _downButtonPressed; }
            set
            {
                if (_downButtonPressed != value)
                {
                    _downButtonPressed = value;
                    OnPropertyChanged();
                }
            }
        }

        public bool LeftButtonPressed
        {
            get { return _leftButtonPressed; }
            set
            {
                if (_leftButtonPressed != value)
                {
                    _leftButtonPressed = value;
                    OnPropertyChanged();
                }
            }
        }

        public bool RightButtonPressed
        {
            get { return _rightButtonPressed; }
            set
            {
                if (_rightButtonPressed != value)
                {
                    _rightButtonPressed = value;
                    OnPropertyChanged();
                }
            }
        }

        public bool FwdButtonPressed
        {
            get { return _fwdButtonPressed; }
            set
            {
                if (_fwdButtonPressed != value)
                {
                    _fwdButtonPressed = value;
                    OnPropertyChanged();
                }
            }
        }

        public bool BackButtonPressed
        {
            get { return _backButtonPressed; }
            set
            {
                if (_backButtonPressed != value)
                {
                    _backButtonPressed = value;
                    OnPropertyChanged();
                }
            }
        }

        public List<string> ComPortList
        {
            get { return _comPortList; }
            set
            {
                _comPortList = value;
                OnPropertyChanged();
            }
        }

        private void ResetProperties()
        {
            _brightness = BrightnessSetupPacket.MinBrightness;
            OnPropertyChanged("Brightness");
            IsAccelerometerOn = false;
            AccelerometerX = 0;
            AccelerometerY = 0;
            AccelerometerZ = 0;
            FwdButtonPressed = BackButtonPressed = false;
            LeftButtonPressed = RightButtonPressed = false;
            UpButtonPressed = DownButtonPressed = false;
        }

        private void OnAccelerometerValuesPacket(object sender, ReceivedComPacketEventArgs e)
        {
            var packet = (AccelerometerValuesPacket)e.Packet;

            AccelerometerX = packet.X;
            AccelerometerY = packet.Y;
            AccelerometerZ = packet.Z;

            var handler = AccelerometerValuesReceived;
            if (handler != null)
                handler(packet.X, packet.Y, packet.Z);
        }

        private void OnAccelerometerStatePacket(object sender, ReceivedComPacketEventArgs e)
        {
            var packet = (AccelerometerStatePacket)e.Packet;
            _isAccelerometerOn = packet.IsAccelerometerOn;
            OnPropertyChanged("IsAccelerometerOn");

            var handler = AccelerometerStateReceived;
            if (handler != null)
                handler(_isAccelerometerOn);
        }

        private void OnBrightnessPacket(object sender, ReceivedComPacketEventArgs e)
        {
            var packet = (BrightnessPacket)e.Packet;
            _brightness = packet.Brightness;
            OnPropertyChanged("Brightness");

            var handler = BrightnessReceived;
            if (handler != null)
                handler(_brightness);
        }

        private void OnButtonStatePacket(object sender, ReceivedComPacketEventArgs e)
        {
            var handler = ButtonStateChanged;
            var packet = (ButtonStatePacket)e.Packet;
            switch(packet.Id)
            {
                case ButtonId.Back:
                    BackButtonPressed = packet.IsPressed;
                    break;

                case ButtonId.Fwd:
                    FwdButtonPressed = packet.IsPressed;
                    break;

                case ButtonId.Left:
                    LeftButtonPressed = packet.IsPressed;
                    break;

                case ButtonId.Right:
                    RightButtonPressed = packet.IsPressed;
                    break;

                case ButtonId.Up:
                    UpButtonPressed = packet.IsPressed;
                    break;

                case ButtonId.Down:
                    DownButtonPressed = packet.IsPressed;
                    break;
            }

            if (handler != null)
                handler(packet.Id, packet.IsPressed);
        }

        private void OnReadyPacket(object sender, ReceivedComPacketEventArgs e)
        {
            _timeoutTimer.Stop();
            CanSendCommand = true;
            var handler = DeviceReady;
            if (handler != null)
                handler();
        }

        private void WritePacket(IComPacketToSend packet)
        {
            CanSendCommand = false;
            _timeoutTimer.Start();
            _port.Write(packet);
        }

#region IDisposable
        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                CloseConnection();
                _timeoutTimer.Dispose();
            }
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
#endregion
    }
}

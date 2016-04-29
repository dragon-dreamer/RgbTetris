// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0
using System;
using System.Collections.ObjectModel;
using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Input;

namespace LedMatrixDebugConsole
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private Logic _logic;
        private RelayCommand<object> _refreshPortListCommand;
        private RelayCommand<object> _startCommunicationCommand;
        private RelayCommand<object> _stopCommunicationCommand;
        private RelayCommand<object> _syncDisplayCommand;
        private RelayCommand<object> _requestBrightnessCommand;
        private RelayCommand<object> _requestAccelerometerValuesCommandCommand;
        private RelayCommand<object> _setNumericDisplayValueCommand;
        private ConcurrentObservableLog _log;
        private bool _accelerometerStateRequested = false;
        private int _numericDisplayValue = 0;

        public MainWindow()
        {
            _logic = new Logic();

            _refreshPortListCommand = new RelayCommand<object>(param => _logic.RefreshPortList(),
                param => !_logic.IsConnectionOpen);
            _startCommunicationCommand = new RelayCommand<object>(param => OpenConnection(),
                param => _logic.CanOpenConnection);
            _stopCommunicationCommand = new RelayCommand<object>(param => CloseConnection(),
                param => _logic.IsConnectionOpen);
            _syncDisplayCommand = new RelayCommand<object>(param => SendDisplayData(),
                param => _logic.CanSendCommand);
            _requestAccelerometerValuesCommandCommand = new RelayCommand<object>(param => RequestAccelerometerValues(),
                param => _logic.CanSendCommand && _logic.IsAccelerometerOn);
            _requestBrightnessCommand = new RelayCommand<object>(param => RequestBrightness(),
                param => _logic.CanSendCommand);
            _setNumericDisplayValueCommand = new RelayCommand<object>(param => SetNumericDisplayValue(),
                param => _logic.CanSendCommand);

            _log = new ConcurrentObservableLog(Dispatcher);

            _logic.DeviceReady += OnDeviceReady;
            _logic.ButtonStateChanged += OnButtonStateChanged;
            _logic.BrightnessReceived += OnBrightnessReceived;
            _logic.AccelerometerStateReceived += OnAccelerometerStateReceived;
            _logic.AccelerometerValuesReceived += OnAccelerometerValuesReceived;
            _logic.EmptyPortList += OnEmptyPortList;
            _logic.ComPortTimeout += OnComPortTimeout;

            _logic.RefreshPortList();

            DataContext = this;
            InitializeComponent();
        }

        public Logic AppLogic
        {
            get { return _logic; }
        }

        public ObservableCollection<LogEntry> Log
        {
            get { return _log.UILog; }
        }

        public string NumericDisplayValue
        {
            get
            {
                return _numericDisplayValue.ToString();
            }

            set
            {
                if (!Regex.IsMatch(value, @"^\d{1,5}$", RegexOptions.ECMAScript))
                    throw new ArgumentException("Valid numeric display values are 0 to 99999");

                _numericDisplayValue = int.Parse(value);
            }
        }

        public ICommand RefreshPortListCommand
        {
            get { return _refreshPortListCommand; }
        }

        public ICommand StartCommunicationCommand
        {
            get { return _startCommunicationCommand; }
        }

        public ICommand StopCommunicationCommand
        {
            get { return _stopCommunicationCommand; }
        }

        public ICommand SyncDisplayCommand
        {
            get { return _syncDisplayCommand; }
        }

        public ICommand RequestAccelerometerValuesCommand
        {
            get { return _requestAccelerometerValuesCommandCommand; }
        }

        public ICommand RequestBrightnessCommand
        {
            get { return _requestBrightnessCommand; }
        }

        public ICommand SetNumericDisplayValueCommand
        {
            get { return _setNumericDisplayValueCommand; }
        }

        private void OnComPortTimeout()
        {
            _log.AddEntry(new LogEntry("Device has not responded. Device is probably offline", LogLevel.Error));
            Dispatcher.BeginInvoke(new Action(() =>
                {
                    _refreshPortListCommand.RaiseCanExecuteChanged();
                    _startCommunicationCommand.RaiseCanExecuteChanged();
                    _stopCommunicationCommand.RaiseCanExecuteChanged();
                }));
        }

        private void OnEmptyPortList()
        {
            _log.AddEntry(new LogEntry("No COM ports found on computer", LogLevel.Warning));
        }

        private void OnAccelerometerValuesReceived(short x, short y, short z)
        {
            _log.AddEntry(new LogEntry("Device accelerometer values: ["
                + x.ToString() + ", " + y.ToString() + ", " + z.ToString() + "]"));
        }

        private void OnAccelerometerStateReceived(bool isAccelerometerOn)
        {
            _log.AddEntry(new LogEntry("Device accelerometer is " + (isAccelerometerOn ? "ON" : "OFF")));
        }

        private void OnBrightnessReceived(int brightness)
        {
            _log.AddEntry(new LogEntry("Device brightness value: " + brightness.ToString()));
        }

        private void OnButtonStateChanged(ButtonId buttonId, bool isPressed)
        {
            _log.AddEntry(new LogEntry("Device button state changed for button " + buttonId.ToString()
                + ": " + (isPressed ? "PRESSED" : "NOT PRESSED")));
        }

        private void OnDeviceReady()
        {
            _log.AddEntry(new LogEntry("Device reported READY"));

            if (!_accelerometerStateRequested)
            {
                _accelerometerStateRequested = true;
                _log.AddEntry(new LogEntry("Requesting device accelerometer state..."));
                _logic.RequestAccelerometerState();
            }
        }

        private void SendDisplayData()
        {
            try
            {
                _logic.SendDisplayData(LedMatrixContents.GetMatrixData());
                _log.AddEntry(new LogEntry("Display data has been sent to the device"));
            }
            catch(Exception e)
            {
                _log.AddEntry(new LogEntry("Cannot send display data: " + e.Message, LogLevel.Error));
            }
        }

        private void RequestBrightness()
        {
            _log.AddEntry(new LogEntry("Requesting device brightness..."));
            _logic.RequestBrightness();
        }

        private void SetNumericDisplayValue()
        {
            _log.AddEntry(new LogEntry("Setting device numeric display value..."));
            _logic.SetNumericDisplayValue(_numericDisplayValue);
        }

        private void OpenConnection()
        {
            try
            {
                _log.AddEntry(new LogEntry("Opening connection to port " + _logic.SelectedComPort));
                _logic.OpenConnection();
                _log.AddEntry(new LogEntry("Connection is open now"));
                RequestBrightness();
            }
            catch(Exception e)
            {
                _log.AddEntry(new LogEntry("Cannot open connection to port: " + e.Message, LogLevel.Error));
            }
        }

        private void CloseConnection()
        {
            _logic.CloseConnection();
            _log.AddEntry(new LogEntry("Connection closed"));
            _accelerometerStateRequested = false;
        }

        private void RequestAccelerometerValues()
        {
            _log.AddEntry(new LogEntry("Requesting device accelerometer values..."));
            _logic.RequestAccelerometerValues();
        }

        private void MainWindowClosing(object sender, EventArgs e)
        {
            DataContext = null;
            _logic.Dispose();
            _logic = null;
        }
    }
}

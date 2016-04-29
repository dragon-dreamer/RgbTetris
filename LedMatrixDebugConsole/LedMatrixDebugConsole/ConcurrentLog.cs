// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0
using System;
using System.Collections.ObjectModel;
using System.Windows.Threading;

namespace LedMatrixDebugConsole
{
    public enum LogLevel
    {
        Info,
        Warning,
        Error
    }

    public class LogEntry
    {
        public LogEntry(string message)
        {
            Text = message;
            Level = LogLevel.Info;
        }

        public LogEntry(string message, LogLevel level)
        {
            Text = message;
            Level = level;
        }

        public string Text { get; set; }
        public LogLevel Level { get; set; }
    }

    public class ConcurrentObservableLog
    {
        private readonly ObservableCollection<LogEntry> _log = new ObservableCollection<LogEntry>();
        private readonly Dispatcher _dispatcher;
        private int _maxEntries = 100;

        public ConcurrentObservableLog(Dispatcher UIDispatcher)
        {
            _dispatcher = UIDispatcher;
        }

        public int MaxSize
        {
            get { return _maxEntries; }
            set
            {
                _maxEntries = value;
                Clear();
            }
        }

        public ObservableCollection<LogEntry> UILog
        {
            get { return _log; }
        }

        public void Clear()
        {
            _dispatcher.BeginInvoke(new Action(() => _log.Clear()));
        }

        public void AddEntry(LogEntry entry)
        {
            _dispatcher.BeginInvoke(new Action(() => {
                if (_log.Count > MaxSize)
                    _log.RemoveAt(0);

                _log.Add(entry);
            }));
        }
    }
}

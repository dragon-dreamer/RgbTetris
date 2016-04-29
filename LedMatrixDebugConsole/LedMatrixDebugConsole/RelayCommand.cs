// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0
using System;
using System.Windows.Input;

namespace LedMatrixDebugConsole
{
    public class RelayCommand : ICommand
    {
        private readonly Action<object> _execute;
        private readonly Predicate<object> _canExecute;
        private EventHandler _canExecuteChanged;

        public RelayCommand(Action<object> execute, Predicate<object> canExecute)
        {
            _execute = execute;
            _canExecute = canExecute;
        }

        public RelayCommand(Action<object> execute)
            : this(execute, null)
        {
        }

        public bool CanExecute(object parameter)
        {
            if (_canExecute == null)
                return true;

            return _canExecute(parameter);
        }
        
        public event EventHandler CanExecuteChanged
        {
            add
            {
                CommandManager.RequerySuggested += value;
                _canExecuteChanged += value;
            }

            remove
            {
                CommandManager.RequerySuggested -= value;
                _canExecuteChanged -= value;
            }
        }

        public void Execute(object parameter)
        {
            _execute(parameter);
        }

        public void RaiseCanExecuteChanged()
        {
            if (_canExecute != null)
                OnCanExecuteChanged();
        }

        protected virtual void OnCanExecuteChanged()
        {
            var handler = _canExecuteChanged;
            if (handler != null)
                handler(this, EventArgs.Empty);
        }
    }
}

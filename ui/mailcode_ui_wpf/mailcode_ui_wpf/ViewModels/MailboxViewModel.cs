using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System;
using System.ComponentModel;
using System.Windows.Controls;
using System.Windows.Threading;

namespace mailcode_ui_wpf.ViewModels
{
    public delegate void BooleanEventHandler(object sender, bool e);

    partial class MailboxViewModel : ObservableObject
    {
        private string? _email;
        private string? _password;
        private string? _serverUrl;
        private string? _serverPort;
        private bool _useSSL;
        private Mailbox? _mailbox;

        public string? Email 
        {
            get { return _email; }
            set
            {
                SetProperty(ref _email, value);
                AddOrUpdateMailboxCommand.NotifyCanExecuteChanged();
            }
        }

        public string? Password 
        {
            get { return _password; }
            set
            {
                SetProperty(ref _password, value);
                AddOrUpdateMailboxCommand.NotifyCanExecuteChanged();
            }
        }

        public string? ServerUrl 
        {
            get { return _serverUrl; }
            set
            {
                AddOrUpdateMailboxCommand.NotifyCanExecuteChanged();
                SetProperty(ref _serverUrl, value);
            }
        }

        public string? ServerPort
        {
            get { return _serverPort; }
            set
            {
                SetProperty(ref _serverPort, value);
                if (!uint.TryParse(value, out _port))
                {
                    _port = 0;
                }
                AddOrUpdateMailboxCommand.NotifyCanExecuteChanged();
            }
        }

        public bool UseSSL 
        {
            get { return _useSSL; }
            set
            {
                SetProperty(ref _useSSL, value);
                AddOrUpdateMailboxCommand.NotifyCanExecuteChanged();
            }
        }

        private uint _port;

        [Browsable(true)]
        [Category("Action")]
        [Description("Invoked when an add, update or delete operation is done")]
        public event BooleanEventHandler OperationDone = delegate { };

        public MailboxViewModel()
        {
        }

        public void SetCurrentMailbox(Mailbox? m)
        {
            if (m != null)
            {
                Email = m.Email;
                Password = m.Password;
                ServerUrl = m.ServerUrl;
                ServerPort = m.Port.ToString();
                _port = m.Port;
                UseSSL = m.UseSSL;
            }
            else
            {
                Email = string.Empty; 
                Password = string.Empty;
                ServerUrl = string.Empty;
                ServerPort = string.Empty;
                _port = 0;
                UseSSL = false;
            }

            _mailbox = m;
        }

        public void NotifyInputChange(object? sender, EventArgs e)
        {
            if (!uint.TryParse(ServerPort, out _port))
            {
                _port = 0;
            }
            AddOrUpdateMailboxCommand.NotifyCanExecuteChanged();
        }

        private bool IsInputValid()
        {
            return !string.IsNullOrEmpty(Email) &&
                   !string.IsNullOrEmpty(Password) &&
                   !string.IsNullOrEmpty(ServerUrl) &&
                   !string.IsNullOrEmpty(ServerPort) &&
                    _port > 0 && _port <= (1 << 16 - 1);
        }

        [RelayCommand(CanExecute = nameof(IsInputValid))]
        private void AddOrUpdateMailbox()
        {
            var mailbox = new Mailbox(Email!, Password!, ServerUrl!, _port, UseSSL);

            var oldMailbox = MailboxRepository.Instance().GetMailboxByEmail(Email!);


            bool result;
            if (oldMailbox != null)
            {
                result = MailboxRepository.Instance().UpdateMailbox(mailbox);
            }
            else
            {
                result = MailboxRepository.Instance().AddMailbox(mailbox);
            }

            if (!result)
            {
                MailboxRepository.Instance().RefreshMailboxList();
            }

            OperationDone(this, result);
        }

        [RelayCommand]
        private void RemoveMailbox()
        {
            if (Email != null)
            {
                bool result = MailboxRepository.Instance().DeleteMailbox(Email);
                if (!result)
                {
                    MailboxRepository.Instance().RefreshMailboxList();
                }

                OperationDone(this, result);
            }
        }
    }
}

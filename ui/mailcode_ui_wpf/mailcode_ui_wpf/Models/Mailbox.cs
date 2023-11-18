using CommunityToolkit.Mvvm.ComponentModel;

namespace mailcode_ui_wpf
{
    public class Mailbox : ObservableObject
    {
        private string email;
        private string password;
        private string serverUrl;
        private uint port;
        private bool useSSL;

        public string Email
        {
            get { return email; }
            set => SetProperty(ref email, value);
        }

        public string Password
        {
            get { return password; }
            set => SetProperty(ref password, value);
        }

        public string ServerUrl
        {
            get { return serverUrl; }
            set => SetProperty(ref serverUrl, value);
        }

        public uint Port
        {
            get { return port; }
            set => SetProperty(ref port, value);
        }

        public bool UseSSL
        {
            get { return useSSL; }
            set => SetProperty(ref useSSL, value);
        }

        public Mailbox(string _email, string _password, string _serverUrl, uint _port, bool _useSSL)
        {
            email = _email;
            password = _password;
            serverUrl = _serverUrl;
            port = _port;
            useSSL = _useSSL;
        }

        public void CopyFrom(Mailbox other)
        {
            Email = other.Email;
            Password = other.Password;
            ServerUrl = other.ServerUrl;
            Port = other.Port;
            UseSSL = other.UseSSL;
        }
    }
}

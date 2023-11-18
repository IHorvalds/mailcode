using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System.Collections.ObjectModel;

namespace mailcode_ui_wpf.ViewModels
{
    partial class MailboxListViewModel : ObservableObject
    {
        public ObservableCollection<Mailbox> Mailboxes { get; set; }

        public MailboxListViewModel()
        {
            Mailboxes = MailboxRepository.Instance().GetMailboxes();
        }

        [RelayCommand]
        private static void FilterMailboxes(object obj)
        {
            string filterString = (string)obj;

            if (filterString == null || filterString == string.Empty)
            {
               MailboxRepository.Instance().SetEmailFilter(string.Empty);
            }
            else
            {
                MailboxRepository.Instance().SetEmailFilter(filterString);
            }
            
            MailboxRepository.Instance().RefreshMailboxList();
        }

    }
}

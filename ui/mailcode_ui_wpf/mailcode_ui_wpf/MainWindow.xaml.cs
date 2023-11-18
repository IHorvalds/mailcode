using mailcode_ui_wpf.ViewModels;
using mailcode_ui_wpf.Views;
using System.Windows;
using System.Windows.Controls;

namespace mailcode_ui_wpf
{
    public partial class MainWindow : Window
    {
        private readonly MailboxListViewModel viewModel = new();
        public Mailbox? selectedMailbox = null;

        public MainWindow()
        {
            InitializeComponent();
            DataContext = viewModel;
        }

        private void UpdateUI()
        {
            MailboxListView.SelectedItem = selectedMailbox;
            mailboxView.CurrentMailbox = selectedMailbox;
        }

        private void MailboxListView_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            selectedMailbox = MailboxListView.SelectedItem as Mailbox;
            UpdateUI();
        }

        private void NewMailbox_Click(object sender, RoutedEventArgs e)
        {
            MailboxListView.SelectedItem = null;
            selectedMailbox = null;
            UpdateUI();
        }

        private void ScrollViewer_PreviewMouseWheel(object sender, System.Windows.Input.MouseWheelEventArgs e)
        {
            ScrollViewer scv = (ScrollViewer)sender;
            const double scrollSensitivity = 10.0;
            
            scv.ScrollToVerticalOffset(scv.VerticalOffset - e.Delta / scrollSensitivity);
            e.Handled = true;
        }

        private void SearchBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            var searchBox = (TextBox)sender;
            if (searchBox != null)
            {
                string filterString = searchBox.Text;

                viewModel.FilterMailboxesCommand.Execute(filterString);
            }
        }
    }
}

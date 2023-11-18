using mailcode_ui_wpf.ViewModels;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Animation;

namespace mailcode_ui_wpf.Views
{
    public partial class MailboxView : UserControl
    {
        private readonly MailboxViewModel ViewModel = new();

        public static readonly DependencyProperty MailboxProperty =
            DependencyProperty.Register("CurrentMailbox", typeof(Mailbox), typeof(MailboxView));

        public Mailbox CurrentMailbox
        {
            get { return (Mailbox)GetValue(MailboxProperty); }
            set 
            { 
                SetValue(MailboxProperty, value);
                ViewModel.SetCurrentMailbox(value);
                if (value == null)
                {
                    saveButton.Content = "Save";
                }
                else
                {
                    saveButton.Content = "Update"; 
                }
            }
        }

        public MailboxView()
        {
            InitializeComponent();
            ViewModel.OperationDone += ViewModel_OperationDone;
            DataContext = ViewModel;
        }

        private void ViewModel_OperationDone(object? sender, bool result)
        {
            StartFadeInOutAnimation(result);
        }

        public void StartFadeInOutAnimation(bool success)
        {
            Storyboard fadeInOutStoryboard = (Storyboard)Resources["FadeInOutAnimation"];
            successErrorBox.Visibility = Visibility.Visible;
            successErrorBox.Opacity = 0;
            if (success)
            {
                successErrorBox.Text = "Success";
                successErrorBox.Background = Brushes.LightGreen;
                successErrorBox.Foreground = Brushes.DarkGreen;
            }
            else
            {
                successErrorBox.Text = "Failed";
                successErrorBox.Background = Brushes.LightPink;
                successErrorBox.Foreground = Brushes.DarkRed;
            }

            fadeInOutStoryboard.AutoReverse = true;
            fadeInOutStoryboard.Completed += (s, args) =>
            {
                successErrorBox.Opacity = 0;
                successErrorBox.Visibility = Visibility.Collapsed;
            };
            
            fadeInOutStoryboard.Begin();
        }
    }
}

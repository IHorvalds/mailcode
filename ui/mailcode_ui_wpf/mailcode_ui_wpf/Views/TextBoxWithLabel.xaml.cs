using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Windows.Controls;
using System.Windows.Data;

namespace mailcode_ui_wpf.Views
{
    public partial class TextBoxWithLabel : UserControl
    {
        public string LabelText
        {
            get { return (string)GetValue(LabelTextProperty); }
            set { SetValue(LabelTextProperty, value); }
        }

        public string TextBoxText
        {
            get { return (string)GetValue(LabelTextProperty); }
            set { SetValue(LabelTextProperty, value); }
        }

        public static readonly DependencyProperty LabelTextProperty =
            DependencyProperty.Register("LabelText", typeof(string), typeof(TextBoxWithLabel));

        public static readonly DependencyProperty TextBoxTextProperty =
            DependencyProperty.Register("TextBoxText", typeof(string), typeof(TextBoxWithLabel));

        [Browsable(true)]
        [Category("Action")]
        [Description("Invoked when the text in the textbox changes")]
        public event EventHandler TextChanged = delegate { };

        public TextBoxWithLabel()
        {
            InitializeComponent();
            label.SetBinding(ContentProperty, new Binding("LabelText") { Source = this });
            textBox.SetBinding(TextBox.TextProperty, new Binding("TextBoxText") { Source = this });
        }

        private void TextBox_KeyUp(object sender, KeyEventArgs e)
        {
            TextChanged(sender, e);
        }
    }
}

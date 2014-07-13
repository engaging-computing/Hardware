using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace PINPoint_v5_0
{
    public partial class FormMessage : Form
    {
        public FormMessage()
        {
            InitializeComponent();
        }

        public string Message
        {
            get
            {
                return textBoxMessage.Text;
            }

            set
            {
                textBoxMessage.Text = value;
            }
        }

        public void AppendText(string text)
        {
            textBoxMessage.AppendText(text);
        }

        private void buttonOK_Click(object sender, EventArgs e)
        {
            Hide();
        }
    }
}
using System;
using System.Net.Sockets;
using System.Text;
using System.Windows;

namespace Editor
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private Socket _socket = null!; // ! - null-forgiving operator
        private readonly string _serverIp = "localhost";
        private readonly int _serverPort = 5234;

        public MainWindow()
        {
            InitializeComponent();
            InitializeSocket();
        }

        private void InitializeSocket()
        {
            try
            {
                _socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
                _socket.Connect(_serverIp, _serverPort);
                MessageBox.Show("Connected to server");
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error while connecting to server: {ex.Message}");
            }
        }

        private void DataSend_OnClick(object sender, RoutedEventArgs e)
        {
            var dataToSend = SendDataTextBox.Text;
            var dataToSendBytes = Encoding.ASCII.GetBytes(dataToSend);
            try
            {
                _socket.Send(dataToSendBytes);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error while sending data: {ex.Message}");
            }
        }
    }
}
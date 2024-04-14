using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.Shapes;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Media;
using AvaloniaEdit.Editing;
using Editor.Utilities;
using MsBox.Avalonia;
using MsBox.Avalonia.Enums;


namespace Editor.Views;

public partial class Editor : Window
{
    private readonly Socket _socket;
    private LinkedList<Paragraph>? _paragraphs;
    private int CaretLine => MainEditor.TextArea.Caret.Line;
    private int CaretColumn => MainEditor.TextArea.Caret.Column;
    private bool _keyHandled;

    public Editor(Socket socket)
    {
        InitializeComponent();
        _socket = socket;
        GetFileFromServer();
        MainEditor.AddHandler(InputElement.KeyDownEvent, Text_KeyDown, RoutingStrategies.Tunnel);
        MainEditor.AddHandler(InputElement.KeyUpEvent, Text_KeyUp, RoutingStrategies.Tunnel);

        // Disable selection of text
        MainEditor.AddHandler(InputElement.PointerMovedEvent,
            (sender, e) => { MainEditor.TextArea.Selection = Selection.Create(MainEditor.TextArea, 0, 0); },
            RoutingStrategies.Tunnel);
    }

    private async void GetFileFromServer()
    {
        // Firstly get metadata from server
        var metadata = new byte[1024];
        await _socket.ReceiveAsync(metadata);
        var metadataString = Encoding.ASCII.GetString(metadata);
        // Parsing the metadata
        // Server metadata format: file_size,ChunkSize
        var metadataArray = metadataString.Split(',');
        int fileSize, chunkSize;
        try
        {
            fileSize = int.Parse(metadataArray[0]);
            chunkSize = int.Parse(metadataArray[1]);
        }
        catch (Exception e)
        {
            await _socket.SendAsync(Encoding.ASCII.GetBytes("Error getting metadata"));
            return;
        }

        await _socket.SendAsync(Encoding.ASCII.GetBytes("OK"));

        var buffer = new byte[chunkSize];
        StringBuilder fileContent = new();

        // Receive the file
        while (fileSize > 0)
        {
            var bytesRead = await _socket.ReceiveAsync(new ArraySegment<byte>(buffer));
            fileSize -= bytesRead;
            fileContent.Append(Encoding.ASCII.GetString(buffer).Replace("\r\n", "\n"));
        }

        _paragraphs = Paragraph.GenerateFromText(fileContent.ToString());
        _paragraphs.ElementAt(1).IsLocked = true;
        OpenFileInEditor();
        AddLockUser("blaquu", Brushes.Red);
        SetLockUserLine("blaquu", 2);
        AddLockUser("andrzejek", Brushes.Blue);
        SetLockUserLine("andrzejek", 4);
    }

    private async Task DisconnectFromServer()
    {
        try
        {
            if (_socket.Connected)
            {
                _paragraphs = null;
                _socket.Shutdown(SocketShutdown.Both);
                _socket.Close();
                await MessageBoxManager.GetMessageBoxStandard(
                    "Success", "Disconnected from server").ShowWindowAsync();
            }
            else
            {
                await MessageBoxManager.GetMessageBoxStandard(
                    "Info", "No active connection to disconnect").ShowWindowAsync();
            }
        }
        catch (Exception ex)
        {
            await MessageBoxManager.GetMessageBoxStandard(
                "Error", "Disconnection from server was unsuccessful. Try again.\n" + ex.Message).ShowWindowAsync();
        }
    }

    private void OpenFileInEditor()
    {
        MainEditor.Text = Paragraph.GetContent(_paragraphs);
    }

    private async void Exit_OnClick(object sender, RoutedEventArgs e)
    {
        var box = MessageBoxManager.GetMessageBoxStandard("", "Are you sure you want to close the editor?",
            ButtonEnum.YesNo);
        var result = await box.ShowAsync();
        if (result != ButtonResult.Yes) return;
        await DisconnectFromServer();
        Close();
    }


    private async void Disconnect_OnClick(object sender, RoutedEventArgs e)
    {
        await DisconnectFromServer();
        new MainMenu().Show();
        Close();
    }

    private void Text_KeyDown(object? sender, KeyEventArgs e)
    {
        if (_paragraphs == null) return;
        if (IsSafeKey(e.Key)) return;

        _keyHandled = false;
        var caretParagraph = _paragraphs.ElementAt(CaretLine - 1);

        if (e.Key == Key.Enter)
        {
            if (!Paragraph.CaretAtTheEndOfLine(caretParagraph, CaretColumn))
            {
                e.Handled = true;
                _keyHandled = true;
                return;
            }

            var newParagraph = new Paragraph { Content = new StringBuilder() };
            var currentNode = _paragraphs.First;
            int counter = 1;
            for (int i = 0; i < CaretLine - 1 && currentNode != null; i++)
            {
                currentNode = currentNode.Next;
                counter++;
            }

            _paragraphs.AddAfter(currentNode!, newParagraph);
            MainEditor.Text = Paragraph.GetContent(_paragraphs);
            MainEditor.TextArea.Caret.Line = counter + 1;
            e.Handled = true;
            _keyHandled = true;
            return;
        }

        if (e.Key == Key.Insert)
        {
            // Toggle of lock
            caretParagraph.IsLocked = !caretParagraph.IsLocked;
            e.Handled = true;
            var caretCopyLine = MainEditor.TextArea.Caret.Line;
            var caretCopyColumn = MainEditor.TextArea.Caret.Column;
            MainEditor.Text = Paragraph.GetContent(_paragraphs);
            MainEditor.TextArea.Caret.Line = caretCopyLine;
            MainEditor.TextArea.Caret.Column = caretCopyColumn;
            return;
        }

        // Basic lock check
        if (caretParagraph.IsLocked)
        {
            e.Handled = true;
            _keyHandled = true;
            return;
        }

        // Special handling of deleting
        if (e.Key == Key.Back)
        {
            if (caretParagraph.Content.Length == 0)
            {
                _paragraphs.Remove(caretParagraph);
                _keyHandled = true;
                return;
            }

            if (CaretColumn == 0)
            {
                e.Handled = true;
                _keyHandled = true;
            }

            return;
        }

        // Disable using del if caret is at the end of the line
        if (e.Key == Key.Delete)
        {
            if (caretParagraph.Content.Length == CaretColumn)
            {
                e.Handled = true;
                _keyHandled = true;
            }
        }
    }

    private void Text_KeyUp(object? sender, KeyEventArgs e)
    {
        if (_paragraphs == null) return;
        if (IsSafeKey(e.Key)) return;
        if (_keyHandled) return;

        // Update the content of the paragraph
        _paragraphs = Paragraph.GenerateFromText(MainEditor.Text, _paragraphs);
    }

    private bool IsSafeKey(Key key)
    {
        // Keys that won't change the content of the paragraph
        return key is Key.Up or Key.Down or Key.Left or Key.Right or Key.Home or Key.End or Key.PageUp or Key.PageDown;
    }

    private void SetLockUserLine(string userName, int columnNumber)
    {
        // Find the Grid for the user
        Grid? userGrid = null;
        foreach (var control in LockIndicatorsPanel.Children)
        {
            var grid = (Grid?)control;
            var textBlock = grid?.Children.OfType<TextBlock>().FirstOrDefault();
            if (textBlock != null && textBlock.Text == userName)
            {
                userGrid = grid;
                break;
            }
        }

        if (userGrid == null)
        {
            // Weird if it happends but just in case
            return;
        }

        // Calculate the new position
        double lineHeight = MainEditor.TextArea.TextView.DefaultLineHeight;
        double yPosition = lineHeight * (columnNumber - 1);

        // Update the position of the Grid
        userGrid.Margin = new Thickness(0, yPosition, 0, 0);
    }

    private void AddLockUser(string userName, ISolidColorBrush color)
    {
        var grid = new Grid();
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });

        var textBlock = new TextBlock
        {
            Text = userName,
            FontFamily = new FontFamily("Cascadia Code,Consolas,Menlo,Monospace"),
            Foreground = Brushes.RoyalBlue,
            VerticalAlignment = Avalonia.Layout.VerticalAlignment.Top,
            HorizontalAlignment = Avalonia.Layout.HorizontalAlignment.Left
        };
        Grid.SetColumn(textBlock, 0);

        var ellipse = new Ellipse
        {
            Width = 15,
            Height = 15,
            Fill = color,
            VerticalAlignment = Avalonia.Layout.VerticalAlignment.Top,
            HorizontalAlignment = Avalonia.Layout.HorizontalAlignment.Left
        };
        Grid.SetColumn(ellipse, 1);

        grid.Children.Add(textBlock);
        grid.Children.Add(ellipse);

        LockIndicatorsPanel.Children.Add(grid);
    }
    
    private async void Save_OnClick(object sender, RoutedEventArgs e)
    {
        if (_paragraphs == null)
        {
            await MessageBoxManager.GetMessageBoxStandard("Error", "No content to save.").ShowWindowAsync();
            return;
        }
        
        var saveFileDialog = new SaveFileDialog();
        saveFileDialog.Title = "Save File";
        saveFileDialog.Filters = new List<FileDialogFilter> { new FileDialogFilter { Name = "Text Files", Extensions = { "txt" } } };

        var result = await saveFileDialog.ShowAsync(this);
        if (result != null)
        {
            try
            {
                using (var writer = new StreamWriter(result))
                {
                    foreach (var paragraph in _paragraphs)
                    {
                        await writer.WriteAsync(paragraph.Content.ToString());
                        await writer.WriteLineAsync();
                    }
                }
                await MessageBoxManager.GetMessageBoxStandard("Success", "File saved successfully.").ShowWindowAsync();
            }
            catch (Exception ex)
            {
                await MessageBoxManager.GetMessageBoxStandard("Error", $"Failed to save file: {ex.Message}").ShowWindowAsync();
            }
        }
    }

    private async void Open_OnClick(object sender, RoutedEventArgs e)
    {
        var openFileDialog = new OpenFileDialog();
        openFileDialog.Title = "Open File";
        openFileDialog.Filters = new List<FileDialogFilter> { new FileDialogFilter { Name = "Text Files", Extensions = { "txt" } } };

        var result = await openFileDialog.ShowAsync(this);
        if (result != null && result.Length > 0)
        {
            var filePath = result[0]; // first argument is the file path
            try
            {
                using (var stream = File.OpenRead(filePath))
                using (var reader = new StreamReader(stream))
                {
                    var fileContent = await reader.ReadToEndAsync();
                    fileContent = fileContent.Replace("\r\n", "\n");
                    _paragraphs = Paragraph.GenerateFromText(fileContent);
                    OpenFileInEditor();
                    await MessageBoxManager.GetMessageBoxStandard("Success", "File opened successfully.").ShowWindowAsync();
                }
            }
            catch (Exception ex)
            {
                await MessageBoxManager.GetMessageBoxStandard("Error", $"Failed to open file: {ex.Message}").ShowWindowAsync();
            }
        }
    }
    private async void New_OnClick(object sender, RoutedEventArgs e)
    {
        var box = MessageBoxManager.GetMessageBoxStandard("", "Are you sure you want to open a new file?",
            ButtonEnum.YesNo);
        var result = await box.ShowAsync();
        if (result != ButtonResult.Yes) return;
        try {
            _paragraphs = new LinkedList<Paragraph>();
            _paragraphs.AddLast(new Paragraph { Content = new StringBuilder() });
            OpenFileInEditor();
            await MessageBoxManager.GetMessageBoxStandard("Success", "New empty file was created.").ShowWindowAsync();
        }
        catch (Exception ex)
        {
            await MessageBoxManager.GetMessageBoxStandard("Error", $"Failed to load a new file: {ex.Message}").ShowWindowAsync();
        }
    }
}
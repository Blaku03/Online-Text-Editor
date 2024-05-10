using Avalonia.Controls;
using Avalonia.Interactivity;

namespace Editor.Views;

public partial class FilePickerWindow : Window
{
    public int PickedOption { get; private set; }
    public string? PickedFilePath { get; private set; }

    public FilePickerWindow()
    {
        InitializeComponent();
    }


    private async void OpenFilePicker_OnClick(object? sender, RoutedEventArgs e)
    {
        var dialog = new OpenFileDialog();
        var result = await dialog.ShowAsync(this);
        if (result != null)
        {
            var filePath = result[0];
            PickedFilePath = filePath;
        }

        PickedOption = 1;
        Close();
    }

    private void SelectDefaultFile_OnClick(object? sender, RoutedEventArgs e)
    {
        PickedOption = 2;
        Close();
    }
}
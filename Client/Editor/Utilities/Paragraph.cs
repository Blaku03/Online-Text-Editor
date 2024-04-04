using System;
using System.Text;

namespace Editor.Utilities;

public class Paragraph
{
    private StringBuilder Content { get; init; } = new();
    public bool IsLocked { get; set; }

    private const string LockedString = " (locked)";

    public static Paragraph[] GetFromText(string fileContent, Paragraph[]? currentParagraphs = null)
    {
        var paragraphsFile = fileContent.Split("\n");

        // Remove \0 from the last paragraph
        paragraphsFile[^1] = paragraphsFile[^1].TrimEnd('\0');

        var paragraphsArr = new Paragraph[paragraphsFile.Length];
        for (var i = 0; i < paragraphsFile.Length; i++)
        {
            paragraphsArr[i] = new Paragraph
                { Content = new StringBuilder(paragraphsFile[i]) };
            if (currentParagraphs != null)
            {
                paragraphsArr[i].IsLocked = currentParagraphs[i].IsLocked;

                // Delete LockedSymbol from the content
                if (paragraphsArr[i].IsLocked)
                {
                    paragraphsArr[i].Content.Remove(paragraphsArr[i].Content.Length - LockedString.Length,
                        LockedString.Length);
                }
            }
        }

        return paragraphsArr;
    }

    public static string GetContent(Paragraph[]? paragraphs)
    {
        if (paragraphs == null) return "";
        var content = new StringBuilder();
        for (int i = 0; i < paragraphs.Length; i++)
        {
            content.Append(paragraphs[i].Content);
            if (paragraphs[i].IsLocked && !content.ToString().EndsWith(LockedString))
            {
                content.Append(LockedString);
            }

            if (i + 1 < paragraphs.Length && !paragraphs[i + 1].Content.ToString().Equals(""))
                content.Append('\n');

            if (paragraphs[i].Content.Equals(""))
                content.Append('\n');
        }

        return content.ToString();
    }

    public static bool AddNewLine(ref Paragraph[] paragraphs, int line, int column)
    {
        // Account column length for LockedString
        column -= paragraphs[line - 1].IsLocked ? LockedString.Length + 1 : 1;

        // Account for negative column
        column = Math.Max(0, column);

        if (column != paragraphs[line - 1].Content.Length && column != 0) return false;

        int lockedLineOffsetModifier = column == 0 ? 1 : 0;

        var newParagraphs = new Paragraph[paragraphs.Length + 1];

        for (var i = 0; i < line - lockedLineOffsetModifier; i++)
        {
            newParagraphs[i] = paragraphs[i];
        }

        newParagraphs[line - lockedLineOffsetModifier] = new Paragraph { Content = new StringBuilder() };
        for (var i = line + 1 - lockedLineOffsetModifier; i < newParagraphs.Length; i++)
        {
            newParagraphs[i] = paragraphs[i - 1];
        }

        paragraphs = newParagraphs;
        return true;
    }
}
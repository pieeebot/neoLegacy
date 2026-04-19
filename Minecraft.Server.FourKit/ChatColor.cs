using System.Text;
using System.Text.RegularExpressions;

namespace Minecraft.Server.FourKit;

/// <summary>
/// All supported color values for chat.
/// </summary>
public class ChatColor
{
    /// <summary>
    /// The special character which prefixes all chat colour codes.
    /// </summary>
    public static readonly char COLOR_CHAR = '\u00A7'; // F

    private static readonly Regex STRIP_COLOR_PATTERN =
        new Regex("(?i)" + COLOR_CHAR + "[0-9A-FK-OR]", RegexOptions.Compiled);

    private static readonly Dictionary<char, ChatColor> BY_CHAR = new();

    /// <summary>Represents black.</summary>
    public static readonly ChatColor BLACK = new('0', false);
    /// <summary>Represents dark blue.</summary>
    public static readonly ChatColor DARK_BLUE = new('1', false);
    /// <summary>Represents dark green.</summary>
    public static readonly ChatColor DARK_GREEN = new('2', false);
    /// <summary>Represents dark blue (aqua).</summary>
    public static readonly ChatColor DARK_AQUA = new('3', false);
    /// <summary>Represents dark red.</summary>
    public static readonly ChatColor DARK_RED = new('4', false);
    /// <summary>Represents dark purple.</summary>
    public static readonly ChatColor DARK_PURPLE = new('5', false);
    /// <summary>Represents gold.</summary>
    public static readonly ChatColor GOLD = new('6', false);
    /// <summary>Represents gray.</summary>
    public static readonly ChatColor GRAY = new('7', false);
    /// <summary>Represents dark gray.</summary>
    public static readonly ChatColor DARK_GRAY = new('8', false);
    /// <summary>Represents blue.</summary>
    public static readonly ChatColor BLUE = new('9', false);
    /// <summary>Represents green.</summary>
    public static readonly ChatColor GREEN = new('a', false);
    /// <summary>Represents aqua.</summary>
    public static readonly ChatColor AQUA = new('b', false);
    /// <summary>Represents red.</summary>
    public static readonly ChatColor RED = new('c', false);
    /// <summary>Represents light purple.</summary>
    public static readonly ChatColor LIGHT_PURPLE = new('d', false);
    /// <summary>Represents yellow.</summary>
    public static readonly ChatColor YELLOW = new('e', false);
    /// <summary>Represents white.</summary>
    public static readonly ChatColor WHITE = new('f', false);
    /// <summary>Resets all previous chat colors or formats.</summary>
    public static readonly ChatColor RESET = new('r', false);

    private readonly char _code;
    private readonly bool _isFormat;
    private readonly string _toString;

    private ChatColor(char code, bool isFormat)
    {
        _code = code;
        _isFormat = isFormat;
        _toString = new string(new[] { COLOR_CHAR, code });
        BY_CHAR[code] = this;
    }

    /// <summary>
    /// Gets the char value associated with this color.
    /// </summary>
    /// <returns>A char value of this color code.</returns>
    public char getChar() => _code;

    /// <summary>
    /// Checks if this code is a format code as opposed to a color code.
    /// </summary>
    /// <returns><c>true</c> if this is a format code.</returns>
    public bool isFormat() => _isFormat;

    /// <summary>
    /// Checks if this code is a color code as opposed to a format code.
    /// </summary>
    /// <returns><c>true</c> if this is a color code.</returns>
    public bool isColor() => !_isFormat && this != RESET;

    /// <summary>
    /// Gets the color represented by the specified color code.
    /// </summary>
    /// <param name="code">Code to check.</param>
    /// <returns>Associative ChatColor with the given code, or null if it doesn't exist.</returns>
    public static ChatColor? getByChar(char code)
    {
        return BY_CHAR.TryGetValue(char.ToLower(code), out var color) ? color : null;
    }

    /// <summary>
    /// Gets the color represented by the specified color code.
    /// </summary>
    /// <param name="code">Code to check.</param>
    /// <returns>Associative ChatColor with the given code, or null if it doesn't exist.</returns>
    public static ChatColor? getByChar(string code)
    {
        if (string.IsNullOrEmpty(code)) return null;
        return getByChar(code[0]);
    }

    /// <summary>
    /// Strips the given message of all color codes.
    /// </summary>
    /// <param name="input">String to strip of color.</param>
    /// <returns>A copy of the input string, without any coloring.</returns>
    public static string? stripColor(string? input)
    {
        if (input == null) return null;
        return STRIP_COLOR_PATTERN.Replace(input, "");
    }

    /// <summary>
    /// Translates a string using an alternate color code character into a string
    /// that uses the internal <see cref="COLOR_CHAR"/> color code character.
    /// The alternate color code character will only be replaced if it is immediately
    /// followed by 0-9, A-F, a-f, K-O, k-o, R or r.
    /// </summary>
    /// <param name="altColorChar">The alternate color code character to replace. Ex: &amp;</param>
    /// <param name="textToTranslate">Text containing the alternate color code character.</param>
    /// <returns>Text containing the <see cref="COLOR_CHAR"/> color code character.</returns>
    public static string translateAlternateColorCodes(char altColorChar, string textToTranslate)
    {
        char[] b = textToTranslate.ToCharArray();
        for (int i = 0; i < b.Length - 1; i++)
        {
            if (b[i] == altColorChar && "0123456789AaBbCcDdEeFfKkLlMmNnOoRr".IndexOf(b[i + 1]) > -1)
            {
                b[i] = COLOR_CHAR;
            }
        }
        return new string(b);
    }

    /// <summary>
    /// Gets the ChatColors used at the end of the given input string.
    /// </summary>
    /// <param name="input">Input string to retrieve the colors from.</param>
    /// <returns>Any remaining ChatColors to pass onto the next line.</returns>
    public static string getLastColors(string input)
    {
        var result = new StringBuilder();
        int length = input.Length;

        for (int index = length - 1; index > -1; index--)
        {
            char section = input[index];
            if (section == COLOR_CHAR && index < length - 1)
            {
                char c = input[index + 1];
                ChatColor? color = getByChar(c);
                if (color != null)
                {
                    result.Insert(0, color.ToString());
                    if (color.isColor() || color == RESET)
                        break;
                }
            }
        }

        return result.ToString();
    }

    public static string operator +(ChatColor color, string text) => color.ToString() + text;

    /// <inheritdoc/>
    public override string ToString() => _toString;
}

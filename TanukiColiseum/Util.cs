using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace TanukiColiseum
{
    class Util
    {
        public static List<string> Split(string s)
        {
            return new List<string>(new Regex("\\s+").Split(s));
        }
    }
}

using System;
using System.Windows.Forms;

namespace TanukiColiseum
{
    class Program
    {
        public void Run(string[] args)
        {
            var options = new Options();
            try
            {
                options.Parse(args);
            }
            catch (ArgumentException e)
            {
                Environment.FailFast("Failed to start TanukiColiseum.", e);
            }

            if (options.Interface == Options.UserInterface.Cli)
            {
                try
                {
                    options.Validate();
                }
                catch (ArgumentException e)
                {
                    Environment.FailFast("Invalid argument(s).", e);
                }

                var cli = new Cli();
                cli.Run(options);
            }
            else
            {
                Application.Run(new Gui());
            }
        }

        [STAThread]
        static void Main(string[] args)
        {
            new Program().Run(args);
        }
    }
}

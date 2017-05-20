using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace TanukiColiseum
{
    public partial class Gui : Form
    {
        public Gui()
        {
            InitializeComponent();

            int numCores = 0;
            foreach (var item in new System.Management.ManagementObjectSearcher("Select * from Win32_Processor").Get())
            {
                numCores += int.Parse(item["NumberOfCores"].ToString());
            }
            textBoxNumConcurrentGames.Text = numCores.ToString();
        }

        private Options ParseOptions()
        {
            var options = new Options();
            options.Engine1FilePath = textBoxEngine1FilePath.Text;
            options.Engine2FilePath = textBoxEngine2FilePath.Text;
            options.Eval1FolderPath = textBoxEval1FolderPath.Text;
            options.Eval2FolderPath = textBoxEval2FolderPath.Text;

            try
            {
                options.NumConcurrentGames = int.Parse(textBoxNumConcurrentGames.Text);
            }
            catch (Exception)
            {
                textBoxOutput.Text = "同時対局数に正しい数字を入力して下さい";
                return null;
            }

            try
            {
                options.NumGames = int.Parse(textBoxNumGames.Text);
            }
            catch (Exception)
            {
                textBoxOutput.Text = "対局数に正しい数字を入力して下さい";
                return null;
            }

            try
            {
                options.HashMb = int.Parse(textBoxHashMb.Text);
            }
            catch (Exception)
            {
                textBoxOutput.Text = "ハッシュサイズに正しい数字を入力して下さい";
                return null;
            }

            try
            {
                options.NumBookMoves1 = int.Parse(textBoxNumBookMoves1.Text);
            }
            catch (Exception)
            {
                textBoxOutput.Text = "思考エンジン1の定跡手数に正しい数字を入力して下さい";
                return null;
            }

            try
            {
                options.NumBookMoves2 = int.Parse(textBoxNumBookMoves2.Text);
            }
            catch (Exception)
            {
                textBoxOutput.Text = "思考エンジン2の定跡手数に正しい数字を入力して下さい";
                return null;
            }

            options.BookFileName1 = textBoxBookFileName1.Text;
            options.BookFileName2 = textBoxBookFileName2.Text;

            try
            {
                options.NumBookMoves = int.Parse(textBoxNumBookMoves.Text);
            }
            catch (Exception)
            {
                textBoxOutput.Text = "開始手数に正しい数字を入力して下さい";
                return null;
            }

            options.SfenFilePath = textBoxSfenFilePath.Text;

            try
            {
                options.TimeMs = int.Parse(textBoxTimeMs.Text);
            }
            catch (Exception)
            {
                textBoxOutput.Text = "思考時間(ms)に正しい数字を入力して下さい";
                return null;
            }

            try
            {
                options.NumNumaNodes = int.Parse(textBoxNumNumaNodes.Text);
            }
            catch (Exception)
            {
                textBoxOutput.Text = "NUMAノード数に正しい数字を入力して下さい";
                return null;
            }

            options.Interface = Options.UserInterface.Gui;
            options.ProgressIntervalMs = 0;
            return options;
        }

        private IEnumerable<Control> GetControls()
        {
            yield return textBoxNumGames;
            yield return textBoxNumConcurrentGames;
            yield return textBoxEngine1FilePath;
            yield return textBoxEval1FolderPath;
            yield return textBoxNumBookMoves1;
            yield return textBoxBookFileName1;
            yield return buttonEval1FolderPath;
            yield return buttonEngine1FilePath;
            yield return textBoxSfenFilePath;
            yield return textBoxHashMb;
            yield return textBoxNumNumaNodes;
            yield return textBoxTimeMs;
            yield return buttonEval2FolderPath;
            yield return buttonEngine2FilePath;
            yield return textBoxBookFileName2;
            yield return textBoxNumBookMoves2;
            yield return textBoxEval2FolderPath;
            yield return textBoxEngine2FilePath;
            yield return textBoxNumBookMoves;
            yield return buttonStart;
        }

        private void EnableControls(bool enabled)
        {
            foreach (var control in GetControls())
            {
                control.Enabled = enabled;
            }
        }

        private delegate void ShowProgressCallback(Status status);

        private void ShowProgress(Status status)
        {
            if (this.InvokeRequired)
            {
                ShowProgressCallback callback = new ShowProgressCallback(ShowProgress);
                this.Invoke(callback, new object[] { status });
                return;
            }

            string engine1 = textBoxEngine1FilePath.Text;
            string engine2 = textBoxEngine2FilePath.Text;
            int numGames = status.NumGames;
            int blackWin = status.Win[0, 0] + status.Win[1, 0];
            int whiteWin = status.Win[0, 1] + status.Win[1, 1];
            int blackWinRatio = 100 * blackWin / (blackWin + whiteWin);
            int whiteWinRatio = 100 * whiteWin / (blackWin + whiteWin);
            int numDraw = status.NumDraw;
            int engine1Win = status.Win[0, 0] + status.Win[0, 1];
            int engine1BlackWin = status.Win[0, 0];
            int engine1WhiteWin = status.Win[0, 1];
            int engine2Win = status.Win[1, 0] + status.Win[1, 1];
            int engine2BlackWin = status.Win[1, 0];
            int engine2WhiteWin = status.Win[1, 1];
            int engine1WinRatio = 100 * engine1Win / (engine1Win + engine2Win);
            int engine2WinRatio = 100 * engine2Win / (engine1Win + engine2Win);
            int engine1BlackWinRatio = 100 * engine1BlackWin / (engine1Win + engine2Win);
            int engine1WhiteWinRatio = 100 * engine1WhiteWin / (engine1Win + engine2Win);
            int engine2BlackWinRatio = 100 * engine2BlackWin / (engine1Win + engine2Win);
            int engine2WhiteWinRatio = 100 * engine2WhiteWin / (engine1Win + engine2Win);
            int numFinishedGames = engine1Win + engine2Win + numDraw;

            string text = string.Format(
                @"対局数{0} 先手勝ち{1}({2}%) 後手勝ち{3}({4}%) 引き分け{5}
{6}
勝ち{7}({8}%) 先手勝ち{9}({10}%) 後手勝ち{11}({12}%)
{13}
勝ち{14}({15}%) 先手勝ち{16}({17}%) 後手勝ち{18}({19}%)",
                numFinishedGames, blackWin, blackWinRatio, whiteWin, whiteWinRatio, numDraw,
                engine1,
                engine1Win, engine1WinRatio, engine1BlackWin, engine1BlackWinRatio, engine1WhiteWin, engine1WhiteWinRatio,
                engine2,
                engine2Win, engine2WinRatio, engine2BlackWin, engine2BlackWinRatio, engine2WhiteWin, engine2WhiteWinRatio);
            textBoxOutput.Text = text;

            progressBar1.Maximum = numGames;
            progressBar1.Value = numFinishedGames;
        }

        private async void buttonStart_Click(object sender, EventArgs e)
        {
            var options = ParseOptions();
            if (options == null)
            {
                return;
            }

            var coliseum = new Coliseum();
            coliseum.OnStatusChanged += ShowProgress;
            EnableControls(false);
            progressBar1.Value = 0;
            textBoxOutput.Text = "(対局準備中です。しばらくお待ちください。)";
            await Task.Run(() => coliseum.Run(options));
            EnableControls(true);
        }

        private void buttonEngine1FilePath_Click(object sender, EventArgs e)
        {
            var dialog = new OpenFileDialog();
            dialog.Filter = "EXEファイル(*.exe)|*.exe";
            dialog.Title = "思考エンジン1を選んで下さい";
            dialog.RestoreDirectory = true;
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                textBoxEngine1FilePath.Text = dialog.FileName;
            }
        }

        private void buttonEval1FolderPath_Click(object sender, EventArgs e)
        {
            var dialog = new FolderBrowserDialog();
            dialog.Description = "思考エンジン1の評価関数フォルダを選んで下さい";
            dialog.SelectedPath = Environment.CurrentDirectory;
            dialog.ShowNewFolderButton = false;
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                textBoxEval1FolderPath.Text = dialog.SelectedPath;
            }
        }

        private void buttonEngine2FilePath_Click(object sender, EventArgs e)
        {
            var dialog = new OpenFileDialog();
            dialog.Filter = "EXEファイル(*.exe)|*.exe";
            dialog.Title = "思考エンジン2を選んで下さい";
            dialog.RestoreDirectory = true;
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                textBoxEngine2FilePath.Text = dialog.FileName;
            }
        }

        private void buttonEval2FolderPath_Click(object sender, EventArgs e)
        {
            var dialog = new FolderBrowserDialog();
            dialog.Description = "思考エンジン2の評価関数フォルダを選んで下さい";
            dialog.SelectedPath = Environment.CurrentDirectory;
            dialog.ShowNewFolderButton = false;
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                textBoxEval2FolderPath.Text = dialog.SelectedPath;
            }
        }
    }
}

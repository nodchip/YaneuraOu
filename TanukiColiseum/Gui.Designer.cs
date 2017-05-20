namespace TanukiColiseum
{
    partial class Gui
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.label1 = new System.Windows.Forms.Label();
            this.textBoxNumGames = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.textBoxNumConcurrentGames = new System.Windows.Forms.TextBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.buttonEval1FolderPath = new System.Windows.Forms.Button();
            this.buttonEngine1FilePath = new System.Windows.Forms.Button();
            this.label7 = new System.Windows.Forms.Label();
            this.textBoxBookFileName1 = new System.Windows.Forms.TextBox();
            this.label6 = new System.Windows.Forms.Label();
            this.textBoxNumBookMoves1 = new System.Windows.Forms.TextBox();
            this.textBoxEval1FolderPath = new System.Windows.Forms.TextBox();
            this.label4 = new System.Windows.Forms.Label();
            this.textBoxEngine1FilePath = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.textBoxSfenFilePath = new System.Windows.Forms.TextBox();
            this.label5 = new System.Windows.Forms.Label();
            this.textBoxHashMb = new System.Windows.Forms.TextBox();
            this.label8 = new System.Windows.Forms.Label();
            this.textBoxNumNumaNodes = new System.Windows.Forms.TextBox();
            this.label9 = new System.Windows.Forms.Label();
            this.textBoxTimeMs = new System.Windows.Forms.TextBox();
            this.label10 = new System.Windows.Forms.Label();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.buttonEval2FolderPath = new System.Windows.Forms.Button();
            this.buttonEngine2FilePath = new System.Windows.Forms.Button();
            this.label11 = new System.Windows.Forms.Label();
            this.textBoxBookFileName2 = new System.Windows.Forms.TextBox();
            this.label12 = new System.Windows.Forms.Label();
            this.textBoxNumBookMoves2 = new System.Windows.Forms.TextBox();
            this.textBoxEval2FolderPath = new System.Windows.Forms.TextBox();
            this.label13 = new System.Windows.Forms.Label();
            this.textBoxEngine2FilePath = new System.Windows.Forms.TextBox();
            this.label14 = new System.Windows.Forms.Label();
            this.textBoxNumBookMoves = new System.Windows.Forms.TextBox();
            this.label15 = new System.Windows.Forms.Label();
            this.progressBar1 = new System.Windows.Forms.ProgressBar();
            this.buttonStart = new System.Windows.Forms.Button();
            this.textBoxOutput = new System.Windows.Forms.TextBox();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(8, 9);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(41, 12);
            this.label1.TabIndex = 0;
            this.label1.Text = "対局数";
            // 
            // textBoxNumGames
            // 
            this.textBoxNumGames.Location = new System.Drawing.Point(83, 6);
            this.textBoxNumGames.Name = "textBoxNumGames";
            this.textBoxNumGames.Size = new System.Drawing.Size(100, 19);
            this.textBoxNumGames.TabIndex = 1;
            this.textBoxNumGames.Text = "1000";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(8, 34);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(65, 12);
            this.label2.TabIndex = 2;
            this.label2.Text = "同時対局数";
            // 
            // textBoxNumConcurrentGames
            // 
            this.textBoxNumConcurrentGames.Location = new System.Drawing.Point(83, 31);
            this.textBoxNumConcurrentGames.Name = "textBoxNumConcurrentGames";
            this.textBoxNumConcurrentGames.Size = new System.Drawing.Size(100, 19);
            this.textBoxNumConcurrentGames.TabIndex = 3;
            this.textBoxNumConcurrentGames.Text = "1";
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.buttonEval1FolderPath);
            this.groupBox1.Controls.Add(this.buttonEngine1FilePath);
            this.groupBox1.Controls.Add(this.label7);
            this.groupBox1.Controls.Add(this.textBoxBookFileName1);
            this.groupBox1.Controls.Add(this.label6);
            this.groupBox1.Controls.Add(this.textBoxNumBookMoves1);
            this.groupBox1.Controls.Add(this.textBoxEval1FolderPath);
            this.groupBox1.Controls.Add(this.label4);
            this.groupBox1.Controls.Add(this.textBoxEngine1FilePath);
            this.groupBox1.Controls.Add(this.label3);
            this.groupBox1.Location = new System.Drawing.Point(10, 81);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(249, 120);
            this.groupBox1.TabIndex = 4;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "思考エンジン1";
            // 
            // buttonEval1FolderPath
            // 
            this.buttonEval1FolderPath.Location = new System.Drawing.Point(225, 37);
            this.buttonEval1FolderPath.Name = "buttonEval1FolderPath";
            this.buttonEval1FolderPath.Size = new System.Drawing.Size(19, 19);
            this.buttonEval1FolderPath.TabIndex = 11;
            this.buttonEval1FolderPath.Text = "...";
            this.buttonEval1FolderPath.UseVisualStyleBackColor = true;
            this.buttonEval1FolderPath.Click += new System.EventHandler(this.buttonEval1FolderPath_Click);
            // 
            // buttonEngine1FilePath
            // 
            this.buttonEngine1FilePath.Location = new System.Drawing.Point(225, 12);
            this.buttonEngine1FilePath.Name = "buttonEngine1FilePath";
            this.buttonEngine1FilePath.Size = new System.Drawing.Size(19, 19);
            this.buttonEngine1FilePath.TabIndex = 10;
            this.buttonEngine1FilePath.Text = "...";
            this.buttonEngine1FilePath.UseVisualStyleBackColor = true;
            this.buttonEngine1FilePath.Click += new System.EventHandler(this.buttonEngine1FilePath_Click);
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(6, 93);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(75, 12);
            this.label7.TabIndex = 9;
            this.label7.Text = "定跡ファイル名";
            // 
            // textBoxBookFileName1
            // 
            this.textBoxBookFileName1.Location = new System.Drawing.Point(119, 90);
            this.textBoxBookFileName1.Name = "textBoxBookFileName1";
            this.textBoxBookFileName1.Size = new System.Drawing.Size(100, 19);
            this.textBoxBookFileName1.TabIndex = 8;
            this.textBoxBookFileName1.Text = "no_book";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(6, 65);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(53, 12);
            this.label6.TabIndex = 7;
            this.label6.Text = "定跡手数";
            // 
            // textBoxNumBookMoves1
            // 
            this.textBoxNumBookMoves1.Location = new System.Drawing.Point(119, 62);
            this.textBoxNumBookMoves1.Name = "textBoxNumBookMoves1";
            this.textBoxNumBookMoves1.Size = new System.Drawing.Size(100, 19);
            this.textBoxNumBookMoves1.TabIndex = 6;
            this.textBoxNumBookMoves1.Text = "0";
            // 
            // textBoxEval1FolderPath
            // 
            this.textBoxEval1FolderPath.Location = new System.Drawing.Point(119, 37);
            this.textBoxEval1FolderPath.Name = "textBoxEval1FolderPath";
            this.textBoxEval1FolderPath.Size = new System.Drawing.Size(100, 19);
            this.textBoxEval1FolderPath.TabIndex = 3;
            this.textBoxEval1FolderPath.Text = "eval";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(6, 40);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(107, 12);
            this.label4.TabIndex = 2;
            this.label4.Text = "評価関数フォルダパス";
            // 
            // textBoxEngine1FilePath
            // 
            this.textBoxEngine1FilePath.Location = new System.Drawing.Point(119, 12);
            this.textBoxEngine1FilePath.Name = "textBoxEngine1FilePath";
            this.textBoxEngine1FilePath.Size = new System.Drawing.Size(100, 19);
            this.textBoxEngine1FilePath.TabIndex = 1;
            this.textBoxEngine1FilePath.Text = "YaneuraOu-2017-early.exe";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(6, 15);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(76, 12);
            this.label3.TabIndex = 0;
            this.label3.Text = "exeファイルパス";
            // 
            // textBoxSfenFilePath
            // 
            this.textBoxSfenFilePath.Location = new System.Drawing.Point(301, 31);
            this.textBoxSfenFilePath.Name = "textBoxSfenFilePath";
            this.textBoxSfenFilePath.Size = new System.Drawing.Size(100, 19);
            this.textBoxSfenFilePath.TabIndex = 8;
            this.textBoxSfenFilePath.Text = "records_2017-05-19.sfen";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(189, 34);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(106, 12);
            this.label5.TabIndex = 7;
            this.label5.Text = "開始局面ファイルパス";
            // 
            // textBoxHashMb
            // 
            this.textBoxHashMb.Location = new System.Drawing.Point(83, 56);
            this.textBoxHashMb.Name = "textBoxHashMb";
            this.textBoxHashMb.Size = new System.Drawing.Size(100, 19);
            this.textBoxHashMb.TabIndex = 6;
            this.textBoxHashMb.Text = "256";
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(8, 59);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(69, 12);
            this.label8.TabIndex = 5;
            this.label8.Text = "ハッシュサイズ";
            // 
            // textBoxNumNumaNodes
            // 
            this.textBoxNumNumaNodes.Location = new System.Drawing.Point(490, 31);
            this.textBoxNumNumaNodes.Name = "textBoxNumNumaNodes";
            this.textBoxNumNumaNodes.Size = new System.Drawing.Size(100, 19);
            this.textBoxNumNumaNodes.TabIndex = 12;
            this.textBoxNumNumaNodes.Text = "1";
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Location = new System.Drawing.Point(407, 34);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(77, 12);
            this.label9.TabIndex = 11;
            this.label9.Text = "NUMAノード数";
            // 
            // textBoxTimeMs
            // 
            this.textBoxTimeMs.Location = new System.Drawing.Point(489, 6);
            this.textBoxTimeMs.Name = "textBoxTimeMs";
            this.textBoxTimeMs.Size = new System.Drawing.Size(100, 19);
            this.textBoxTimeMs.TabIndex = 10;
            this.textBoxTimeMs.Text = "1000";
            // 
            // label10
            // 
            this.label10.AutoSize = true;
            this.label10.Location = new System.Drawing.Point(407, 9);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(76, 12);
            this.label10.TabIndex = 9;
            this.label10.Text = "思考時間(ms)";
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.buttonEval2FolderPath);
            this.groupBox2.Controls.Add(this.buttonEngine2FilePath);
            this.groupBox2.Controls.Add(this.label11);
            this.groupBox2.Controls.Add(this.textBoxBookFileName2);
            this.groupBox2.Controls.Add(this.label12);
            this.groupBox2.Controls.Add(this.textBoxNumBookMoves2);
            this.groupBox2.Controls.Add(this.textBoxEval2FolderPath);
            this.groupBox2.Controls.Add(this.label13);
            this.groupBox2.Controls.Add(this.textBoxEngine2FilePath);
            this.groupBox2.Controls.Add(this.label14);
            this.groupBox2.Location = new System.Drawing.Point(265, 81);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(249, 120);
            this.groupBox2.TabIndex = 12;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "思考エンジン2";
            // 
            // buttonEval2FolderPath
            // 
            this.buttonEval2FolderPath.Location = new System.Drawing.Point(225, 37);
            this.buttonEval2FolderPath.Name = "buttonEval2FolderPath";
            this.buttonEval2FolderPath.Size = new System.Drawing.Size(19, 19);
            this.buttonEval2FolderPath.TabIndex = 11;
            this.buttonEval2FolderPath.Text = "...";
            this.buttonEval2FolderPath.UseVisualStyleBackColor = true;
            this.buttonEval2FolderPath.Click += new System.EventHandler(this.buttonEval2FolderPath_Click);
            // 
            // buttonEngine2FilePath
            // 
            this.buttonEngine2FilePath.Location = new System.Drawing.Point(225, 12);
            this.buttonEngine2FilePath.Name = "buttonEngine2FilePath";
            this.buttonEngine2FilePath.Size = new System.Drawing.Size(19, 19);
            this.buttonEngine2FilePath.TabIndex = 10;
            this.buttonEngine2FilePath.Text = "...";
            this.buttonEngine2FilePath.UseVisualStyleBackColor = true;
            this.buttonEngine2FilePath.Click += new System.EventHandler(this.buttonEngine2FilePath_Click);
            // 
            // label11
            // 
            this.label11.AutoSize = true;
            this.label11.Location = new System.Drawing.Point(6, 93);
            this.label11.Name = "label11";
            this.label11.Size = new System.Drawing.Size(75, 12);
            this.label11.TabIndex = 9;
            this.label11.Text = "定跡ファイル名";
            // 
            // textBoxBookFileName2
            // 
            this.textBoxBookFileName2.Location = new System.Drawing.Point(119, 90);
            this.textBoxBookFileName2.Name = "textBoxBookFileName2";
            this.textBoxBookFileName2.Size = new System.Drawing.Size(100, 19);
            this.textBoxBookFileName2.TabIndex = 8;
            this.textBoxBookFileName2.Text = "no_book";
            // 
            // label12
            // 
            this.label12.AutoSize = true;
            this.label12.Location = new System.Drawing.Point(6, 65);
            this.label12.Name = "label12";
            this.label12.Size = new System.Drawing.Size(53, 12);
            this.label12.TabIndex = 7;
            this.label12.Text = "定跡手数";
            // 
            // textBoxNumBookMoves2
            // 
            this.textBoxNumBookMoves2.Location = new System.Drawing.Point(119, 62);
            this.textBoxNumBookMoves2.Name = "textBoxNumBookMoves2";
            this.textBoxNumBookMoves2.Size = new System.Drawing.Size(100, 19);
            this.textBoxNumBookMoves2.TabIndex = 6;
            this.textBoxNumBookMoves2.Text = "0";
            // 
            // textBoxEval2FolderPath
            // 
            this.textBoxEval2FolderPath.Location = new System.Drawing.Point(119, 37);
            this.textBoxEval2FolderPath.Name = "textBoxEval2FolderPath";
            this.textBoxEval2FolderPath.Size = new System.Drawing.Size(100, 19);
            this.textBoxEval2FolderPath.TabIndex = 3;
            this.textBoxEval2FolderPath.Text = "eval";
            // 
            // label13
            // 
            this.label13.AutoSize = true;
            this.label13.Location = new System.Drawing.Point(6, 40);
            this.label13.Name = "label13";
            this.label13.Size = new System.Drawing.Size(107, 12);
            this.label13.TabIndex = 2;
            this.label13.Text = "評価関数フォルダパス";
            // 
            // textBoxEngine2FilePath
            // 
            this.textBoxEngine2FilePath.Location = new System.Drawing.Point(119, 12);
            this.textBoxEngine2FilePath.Name = "textBoxEngine2FilePath";
            this.textBoxEngine2FilePath.Size = new System.Drawing.Size(100, 19);
            this.textBoxEngine2FilePath.TabIndex = 1;
            this.textBoxEngine2FilePath.Text = "YaneuraOu-2017-early.exe";
            // 
            // label14
            // 
            this.label14.AutoSize = true;
            this.label14.Location = new System.Drawing.Point(6, 15);
            this.label14.Name = "label14";
            this.label14.Size = new System.Drawing.Size(76, 12);
            this.label14.TabIndex = 0;
            this.label14.Text = "exeファイルパス";
            // 
            // textBoxNumBookMoves
            // 
            this.textBoxNumBookMoves.Location = new System.Drawing.Point(301, 6);
            this.textBoxNumBookMoves.Name = "textBoxNumBookMoves";
            this.textBoxNumBookMoves.Size = new System.Drawing.Size(100, 19);
            this.textBoxNumBookMoves.TabIndex = 14;
            this.textBoxNumBookMoves.Text = "24";
            // 
            // label15
            // 
            this.label15.AutoSize = true;
            this.label15.Location = new System.Drawing.Point(189, 9);
            this.label15.Name = "label15";
            this.label15.Size = new System.Drawing.Size(53, 12);
            this.label15.TabIndex = 13;
            this.label15.Text = "開始手数";
            // 
            // progressBar1
            // 
            this.progressBar1.Location = new System.Drawing.Point(10, 207);
            this.progressBar1.Name = "progressBar1";
            this.progressBar1.Size = new System.Drawing.Size(574, 23);
            this.progressBar1.Style = System.Windows.Forms.ProgressBarStyle.Continuous;
            this.progressBar1.TabIndex = 15;
            // 
            // buttonStart
            // 
            this.buttonStart.Location = new System.Drawing.Point(509, 238);
            this.buttonStart.Name = "buttonStart";
            this.buttonStart.Size = new System.Drawing.Size(75, 23);
            this.buttonStart.TabIndex = 16;
            this.buttonStart.Text = "対局開始";
            this.buttonStart.UseVisualStyleBackColor = true;
            this.buttonStart.Click += new System.EventHandler(this.buttonStart_Click);
            // 
            // textBoxOutput
            // 
            this.textBoxOutput.Location = new System.Drawing.Point(10, 238);
            this.textBoxOutput.Multiline = true;
            this.textBoxOutput.Name = "textBoxOutput";
            this.textBoxOutput.Size = new System.Drawing.Size(493, 80);
            this.textBoxOutput.TabIndex = 18;
            this.textBoxOutput.Text = "(ここに結果が表示されます)";
            // 
            // Gui
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(596, 327);
            this.Controls.Add(this.textBoxOutput);
            this.Controls.Add(this.buttonStart);
            this.Controls.Add(this.progressBar1);
            this.Controls.Add(this.textBoxNumBookMoves);
            this.Controls.Add(this.label15);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.textBoxNumNumaNodes);
            this.Controls.Add(this.label9);
            this.Controls.Add(this.textBoxTimeMs);
            this.Controls.Add(this.label10);
            this.Controls.Add(this.textBoxSfenFilePath);
            this.Controls.Add(this.label5);
            this.Controls.Add(this.textBoxHashMb);
            this.Controls.Add(this.label8);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.textBoxNumConcurrentGames);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.textBoxNumGames);
            this.Controls.Add(this.label1);
            this.Name = "Gui";
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.Text = "Gui";
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox textBoxNumGames;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox textBoxNumConcurrentGames;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TextBox textBoxEngine1FilePath;
        private System.Windows.Forms.TextBox textBoxEval1FolderPath;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.TextBox textBoxNumBookMoves1;
        private System.Windows.Forms.TextBox textBoxBookFileName1;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Button buttonEval1FolderPath;
        private System.Windows.Forms.Button buttonEngine1FilePath;
        private System.Windows.Forms.TextBox textBoxSfenFilePath;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.TextBox textBoxHashMb;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.TextBox textBoxNumNumaNodes;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.TextBox textBoxTimeMs;
        private System.Windows.Forms.Label label10;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Button buttonEval2FolderPath;
        private System.Windows.Forms.Button buttonEngine2FilePath;
        private System.Windows.Forms.Label label11;
        private System.Windows.Forms.TextBox textBoxBookFileName2;
        private System.Windows.Forms.Label label12;
        private System.Windows.Forms.TextBox textBoxNumBookMoves2;
        private System.Windows.Forms.TextBox textBoxEval2FolderPath;
        private System.Windows.Forms.Label label13;
        private System.Windows.Forms.TextBox textBoxEngine2FilePath;
        private System.Windows.Forms.Label label14;
        private System.Windows.Forms.TextBox textBoxNumBookMoves;
        private System.Windows.Forms.Label label15;
        private System.Windows.Forms.ProgressBar progressBar1;
        private System.Windows.Forms.Button buttonStart;
        private System.Windows.Forms.TextBox textBoxOutput;
    }
}
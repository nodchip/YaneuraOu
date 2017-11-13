set terminal png
set datafile separator ","
set grid

set output "00_rmse_value.png"
set title "rmse value"
plot "loss.csv" using 1:2 with lines title "train" , \
     "loss.csv" using 1:7 with lines title "test"

set output "01_rmse_winning_percentage.png"
set title "rmse winning percentage"
plot "loss.csv" using 1:3 with lines title "train" , \
     "loss.csv" using 1:8 with lines title "test"

set output "02_rmse_win_lose.png"
set title "rmse win lose"
plot "loss.csv" using 1:13 with lines title "train" , \
     "loss.csv" using 1:14 with lines title "test"

set output "03_mean_cross_entropy.png"
set title "mean cross entropy"
plot "loss.csv" using 1:4 with lines title "train" , \
     "loss.csv" using 1:9 with lines title "test"

set output "04_mean_cross_entropy_eval.png"
set title "mean cross entropy eval"
plot "loss.csv" using 1:5 with lines title "train" , \
     "loss.csv" using 1:10 with lines title "test"

set output "05_mean_cross_entropy_win.png"
set title "mean cross entropy win"
plot "loss.csv" using 1:6 with lines title "train" , \
     "loss.csv" using 1:11 with lines title "test"

set output "06_mean_cross_entropy_eval_win.png"
set title "mean cross entropy eval win"
plot "loss.csv" using 1:5 with lines title "train eval" , \
     "loss.csv" using 1:10 with lines title "test eval" , \
     "loss.csv" using 1:6 with lines title "train win" , \
     "loss.csv" using 1:11 with lines title "test win"

set output "07_norm.png"
set title "norm"
plot "loss.csv" using 1:12 with lines title "norm"

set output "08_mean_entropy_eval.png"
set title "mean entropy eval"
plot "loss.csv" using 1:15 with lines title "train" , \
     "loss.csv" using 1:17 with lines title "test"

set output "09_mean_kld_eval.png"
set title "mean kld eval"
plot "loss.csv" using 1:16 with lines title "train" , \
     "loss.csv" using 1:18 with lines title "test"
	
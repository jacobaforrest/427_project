set terminal png size 1920,1080
set output "tcp_cwnd.png"
plot "tcp_cwnd.dat" using 1:2 title 'Congestion Window' with linespoints
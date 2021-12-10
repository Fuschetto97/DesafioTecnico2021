#sarx.conf
set terminal png size 1600,600

set terminal png truecolor
set output "Temperatura"
set xdata time
set timefmt "%H:%M:%S"
set style data lines
set grid
plot "sarx.txt" using 1:2 title "Temperatura [C]"

set terminal png truecolor
set output "Presion"
set xdata time
set timefmt "%H:%M:%S"
set style data lines
set grid
plot "sarx.txt" using 1:3 title "Presion [hPa]"

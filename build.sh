latexmk -C -xelatex -shell-escape 
latexmk -pvc -xelatex -shell-escape main.tex
# latexmk -p -xelatex -shell-escape -outdir=./build main.tex &&
# pdfunite ./extra/title_page.pdf ./build/main.pdf  ./build/Отчет.pdf
# zathura ./build/Отчет.pdf
# xelatex -shell-escape -outdir=./build main.tex

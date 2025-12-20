#!/bin/bash
# Скрипт для конвертации BOM_and_Wiring.md в PDF

echo "=== Smart Areometr: Конвертация BOM в PDF ==="
echo ""

# Проверка наличия файла
if [ ! -f "BOM_and_Wiring.md" ]; then
    echo "❌ Ошибка: Файл BOM_and_Wiring.md не найден!"
    exit 1
fi

echo "✅ Файл BOM_and_Wiring.md найден"
echo ""

# Метод 1: Попробовать pandoc
if command -v pandoc &> /dev/null; then
    echo "📄 Найден pandoc, конвертируем с помощью pandoc..."
    pandoc BOM_and_Wiring.md \
        -o BOM_and_Wiring.pdf \
        --pdf-engine=xelatex \
        -V geometry:margin=2cm \
        -V fontsize=11pt \
        -V mainfont="DejaVu Sans" \
        --toc \
        --toc-depth=2 \
        -V lang=ru \
        2>/dev/null

    if [ $? -eq 0 ]; then
        echo "✅ PDF создан: BOM_and_Wiring.pdf"
        exit 0
    else
        echo "⚠️ Pandoc завершился с ошибкой, пробуем другие методы..."
    fi
fi

# Метод 2: Попробовать markdown-pdf (npm)
if command -v markdown-pdf &> /dev/null; then
    echo "📄 Найден markdown-pdf, конвертируем..."
    markdown-pdf BOM_and_Wiring.md -o BOM_and_Wiring.pdf 2>/dev/null

    if [ $? -eq 0 ]; then
        echo "✅ PDF создан: BOM_and_Wiring.pdf"
        exit 0
    fi
fi

# Метод 3: Попробовать wkhtmltopdf
if command -v wkhtmltopdf &> /dev/null; then
    echo "📄 Найден wkhtmltopdf, конвертируем через HTML..."

    # Создаем временный HTML
    cat > /tmp/bom_temp.html << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        body { font-family: Arial, sans-serif; margin: 2cm; }
        h1 { color: #2c3e50; border-bottom: 3px solid #3498db; }
        h2 { color: #34495e; border-bottom: 2px solid #95a5a6; }
        table { border-collapse: collapse; width: 100%; margin: 20px 0; }
        th { background-color: #3498db; color: white; padding: 10px; }
        td { border: 1px solid #ddd; padding: 8px; }
        pre { background: #f4f4f4; padding: 10px; border-left: 4px solid #3498db; }
    </style>
</head>
<body>
EOF

    # Конвертируем markdown в HTML (базовая конвертация)
    sed -E 's/^# (.+)/<h1>\1<\/h1>/g' BOM_and_Wiring.md | \
    sed -E 's/^## (.+)/<h2>\1<\/h2>/g' | \
    sed -E 's/^### (.+)/<h3>\1<\/h3>/g' | \
    sed -E 's/\*\*(.+)\*\*/<strong>\1<\/strong>/g' >> /tmp/bom_temp.html

    echo "</body></html>" >> /tmp/bom_temp.html

    wkhtmltopdf /tmp/bom_temp.html BOM_and_Wiring.pdf 2>/dev/null
    rm /tmp/bom_temp.html

    if [ $? -eq 0 ]; then
        echo "✅ PDF создан: BOM_and_Wiring.pdf"
        exit 0
    fi
fi

# Если ничего не сработало
echo ""
echo "❌ Автоматическая конвертация не удалась."
echo ""
echo "📋 ИНСТРУКЦИИ ПО РУЧНОЙ КОНВЕРТАЦИИ:"
echo ""
echo "ВАРИАНТ 1: Через браузер (самый простой)"
echo "  1. Откройте файл BOM_and_Wiring.md в VSCode/любом редакторе markdown"
echo "  2. Используйте предварительный просмотр (Ctrl+Shift+V в VSCode)"
echo "  3. Правой кнопкой → Печать → Сохранить как PDF"
echo ""
echo "ВАРИАНТ 2: Установить pandoc"
echo "  Ubuntu/Debian:"
echo "    sudo apt-get install pandoc texlive-xetex"
echo "    ./convert_to_pdf.sh"
echo ""
echo "  macOS:"
echo "    brew install pandoc mactex"
echo "    ./convert_to_pdf.sh"
echo ""
echo "ВАРИАНТ 3: Онлайн конвертация"
echo "  1. Откройте https://www.markdowntopdf.com/"
echo "  2. Загрузите файл BOM_and_Wiring.md"
echo "  3. Скачайте готовый PDF"
echo ""
echo "ВАРИАНТ 4: GitHub"
echo "  1. Закоммитьте BOM_and_Wiring.md в репозиторий"
echo "  2. Откройте на GitHub - там будет красивый рендеринг"
echo "  3. Используйте расширение браузера для сохранения в PDF"
echo ""

exit 1

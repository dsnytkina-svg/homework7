// homework7.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>     // для cout, cerr
#include <fstream>      // для ifstream (чтение) и ofstream (запись)
#include <vector>       // для динамического массива vector
#include <cmath>        // для sqrt
#include <algorithm>    // для sort
#include <chrono>       // для измерения времени
#include <iomanip>      // для fixed и setprecision
#include <locale>

#ifdef _OPENMP          // если компилятор поддерживает OpenMP
#include <omp.h>        // подключаем функции OpenMP
#endif

int main() {
    setlocale(LC_ALL, "RUS");

    const double eps = 1e-9;

    // ---------- 1. ЧИТАЕМ МАССИВ ИЗ ФАЙЛА ----------
    std::ifstream fin("input.txt");          // открываем входной файл для чтения
    if (!fin.is_open()) {                    // проверяем, что он открылся
        std::cerr << "Ошибка: не удалось открыть input.txt\n";
        return 1;
    }

    std::vector<double> data;   // сюда сложим все числа из файла
    double x;                   // временная переменная для одного числа
    while (fin >> x) {          // читаем числа по одному, пока файл не кончится
        data.push_back(x);      // добавляем прочитанное число в конец вектора
    }
    fin.close();                // входной файл больше не нужен — закрываем

    if (data.empty()) {         // если в файле не оказалось ни одного числа
        std::cerr << "Ошибка: input.txt пуст.\n";
        return 1;
    }

    size_t N = data.size();     // запоминаем количество элементов

    // ---------- 2. ОТКРЫВАЕМ ФАЙЛ ДЛЯ ЗАПИСИ РЕЗУЛЬТАТА ----------
    std::ofstream fout("output.txt", std::ios::app);  // дописываем в конец, старое не стираем
    if (!fout.is_open()) {
        std::cerr << "Ошибка: не удалось открыть output.txt\n";
        return 1;
    }

    // форматирование: 6 знаков после запятой и в консоли, и в файле
    std::cout << std::fixed << std::setprecision(6);
    fout << std::fixed << std::setprecision(6);

    fout << "=== Новая обработка ===\n";
    fout << "Количество элементов: " << N << "\n";

    // ---------- 3. СОРТИРУЕМ МАССИВ ----------
    std::sort(data.begin(), data.end());   // стандартная сортировка по возрастанию

    // выводим отсортированный массив и в консоль, и в файл
    std::cout << "Отсортированный массив: ";
    fout << "Отсортированный массив:\n";
    for (size_t i = 0; i < N; ++i) {
        std::cout << data[i] << " ";
        fout << data[i] << " ";
        if ((i + 1) % 10 == 0) fout << "\n";  // в файле перенос строки каждые 10 чисел — чтобы не было одной длинной строки
    }
    std::cout << "\n";
    fout << "\n";

    // ---------- 4. ВЫБОР КОЛИЧЕСТВА ПОТОКОВ ----------
   // Задание: «распараллелить на ЛЮБОЕ количество потоков» —
   // поэтому список можно менять: допиши любое число, например {1, 2, 4, 8}
    std::vector<int> threads_list = { 1, 2, 4 };

    // Сюда сохраним время для каждого варианта, чтобы потом построить таблицу сравнения
    std::vector<double> times;

    // ---------- 5. ПАРАЛЛЕЛЬНОЕ ВЫЧИСЛЕНИЕ СРЕДНЕГО И СКО ----------
    for (int threads : threads_list) {
#ifdef _OPENMP
        omp_set_num_threads(threads);   // задаём число потоков для этого прогона
#endif
        auto start = std::chrono::high_resolution_clock::now();  // старт замера

        // Шаг 1: сумма всех элементов (для среднего)
        double sum = 0.0;
#pragma omp parallel for reduction(+:sum)   // параллельный цикл; reduction собирает частные суммы потоков в одну
        for (size_t i = 0; i < N; ++i) {
            sum += data[i];
        }
        double mean = sum / static_cast<double>(N);   // среднее арифметическое

        // Шаг 2: сумма квадратов отклонений (для СКО)
        double sq_sum = 0.0;
#pragma omp parallel for reduction(+:sq_sum)
        for (size_t i = 0; i < N; ++i) {
            double diff = data[i] - mean;   // отклонение элемента от среднего
            sq_sum += diff * diff;          // его квадрат
        }
        double variance = sq_sum / static_cast<double>(N);  // дисперсия
        double stddev = std::sqrt(variance);                // среднеквадратическое отклонение

        auto end = std::chrono::high_resolution_clock::now();    // конец замера
        std::chrono::duration<double> elapsed = end - start;
        times.push_back(elapsed.count());   // запоминаем время этого прогона

        // Результаты — в файл
        fout << "Количество потоков: " << threads << "\n";
        fout << "  Среднее арифметическое:            " << mean << "\n";
        fout << "  Среднеквадратическое отклонение:   " << stddev << "\n";
        fout << "  Время вычисления:                  " << elapsed.count() << " сек\n";
    }

    // ---------- 6. ДЕМОНСТРАЦИЯ РАЗНИЦЫ ВО ВРЕМЕНИ ----------
    // Таблица: потоки, время, ускорение относительно 1 потока
    fout << "\nСравнение времени выполнения:\n";
    fout << "Потоки | Время (сек) | Ускорение\n";
    for (size_t k = 0; k < threads_list.size(); ++k) {
        double speedup = (k == 0 || times[0] == 0.0)
            ? 1.0
            : times[0] / times[k];   // ускорение = время(1 поток) / время(N потоков)
        fout << threads_list[k]
            << "      | " << times[k]
            << "      | x" << speedup << "\n";
    }

    // Дублируем таблицу в консоль, чтобы видеть результат сразу
    std::cout << "\nСравнение времени выполнения:\n";
    for (size_t k = 0; k < threads_list.size(); ++k) {
        std::cout << "Потоков: " << threads_list[k]
            << " | Время: " << times[k] << " сек"
            << " | Ускорение: x" << (k == 0 ? 1.0 : times[0] / times[k]) << "\n";
    }

    fout << "-------------------\n\n";
    fout.close();

    std::cout << "Готово! Отчёт записан в output.txt\n";
    return 0;
}
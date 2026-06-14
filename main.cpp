#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <fstream>
#include <algorithm>
#include <functional>
#include <iomanip>
#include <type_traits>

// мы приняли решение
using namespace std;

string to_csv_number(double value) {
    // Преобразуем double в строку с фиксированной точностью,
    // чтобы избежать научной нотации и лишних нулей
    ostringstream oss;
    oss << fixed << setprecision(6) << value;
    string s = oss.str();
    // Удаляем завершающие нули и точку, если она последняя
    while (s.size() > 1 && s.back() == '0') s.pop_back();
    if (s.back() == '.') s.pop_back();
    // Заменяем десятичную точку на запятую
    replace(s.begin(), s.end(), '.', ',');
    return s;
}

// интерфейс для построения чего-то полезного
template<typename T>
class RangeQueryStructure
{
public:
    virtual ~RangeQueryStructure() = default;

    // построение структуры из массива
    virtual void build(const vector<T>& arr) = 0;

    // запросик на отрезке
    virtual T query(int l, int r) = 0;

    virtual void update(int idx, T val)
    {
        throw std::logic_error("Update not supported for this structure");
    }

    // геттеры для счётчиков операций
    virtual int64_t getBuildOps() const = 0;
    virtual int64_t getQueryOps() const = 0;
    virtual int64_t getUpdateOps() const { return 0; }

    // имя структуры (для вывода)
    virtual string getName() const = 0;
};


// одномерные префиксные суммы
// просто создаем новый массив такого же размера, i-тый элемент - сумма от 0го до iго
// Асимптотика по "методичке": О(N) - построение, О(1) - запрос, обновление имеет асимптотику О(N) в общем виде, отдельной функции не стоит
template<typename T>
class PrefixSum1D : public RangeQueryStructure<T>
{
private:
    vector<T> prefix;                               // префиксные суммы
    mutable int64_t build_ops = 0;
    mutable int64_t query_ops = 0;

public:
    void build(const vector<T>& arr) override
    {
        int n = arr.size();
        prefix.assign(n + 1, 0);

        for (int i = 0; i < n; i++)                 // считаем сумму массива префикс как сумму прошлых элементов входного массива + считаем количество операций
        {
            prefix[i + 1] = prefix[i] + arr[i];
            build_ops++;                            // доминирующая(ух как доминирует) операция: одно сложение на итерацию
        }
    }

    T query(int l, int r) override
    {
        if (l < 0) l = 0;
        if (r >= (int)prefix.size() - 1) r = prefix.size() - 2;
        if (l > r) return 0;

        query_ops++;                                // доминирующая операция: одно вычитание
        return prefix[r + 1] - prefix[l];           // считаем быстренько
    }

    int64_t getBuildOps() const override { return build_ops; }
    int64_t getQueryOps() const override { return query_ops; }

    string getName() const override { return "1D Prefix Sum (RSQ)"; }
};


// двумерн(ия)ые префиксные суммы ууууу
// просто создаем новую матрицу такого же размера, элемент (i, j) - сумма прямоугольника от (0, 0) до нужного
// Асимптотика по "методичке": О(N*M) - построение, О(1) - запрос, обновление имеет асимптотику О(N*M) в общем виде, отдельной функции не стоит
template<typename T>
class PrefixSum2D : public RangeQueryStructure<T>
{
private:
    vector<vector<T>> prefix;                       // 2D префиксные суммы
    int rows = 0, cols = 0;
    mutable int64_t build_ops = 0;
    mutable int64_t query_ops = 0;

public:
    void build(const vector<T>& arr) override       // Для 2D этот метод не используется, для гениев
    {
        throw std::logic_error("2D structure requires 2D array");
    }

    // считаем по-особенному
    void build2D(const vector<vector<T>>& matrix)
    {
        rows = matrix.size();
        if (rows == 0) return;
        cols = matrix[0].size();

        prefix.assign(rows + 1, vector<T>(cols + 1, 0));

        for (int i = 0; i < rows; i++)
        {
            for (int j = 0; j < cols; j++)
            {
                prefix[i + 1][j + 1] = matrix[i][j] + prefix[i][j + 1] + prefix[i + 1][j] - prefix[i][j]; // ого для эго есть какая-то сложная (нет) формула
                build_ops++;                    // доминирующая операция: вычисление одной ячейки
            }
        }
    }

    // запрос суммы в прямоугольнике [(x1,y1), (x2,y2)]
    T query2D(int x1, int y1, int x2, int y2)
    {
        if (x1 < 0) x1 = 0;
        if (y1 < 0) y1 = 0;
        if (x2 >= rows) x2 = rows - 1;
        if (y2 >= cols) y2 = cols - 1;
        if (x1 > x2 || y1 > y2) return 0;

        query_ops++;                                 // доминирующая операция: вычисление ответа по 4 углам
        return prefix[x2 + 1][y2 + 1] - prefix[x1][y2 + 1] - prefix[x2 + 1][y1] + prefix[x1][y1];    //тоже очень сложная формула
    }

    T query(int l, int r) override
    {
        throw std::logic_error("Use query2D for 2D structure");
    }

    int64_t getBuildOps() const override { return build_ops; }
    int64_t getQueryOps() const override { return query_ops; }

    string getName() const override { return "2D Prefix Sum (RSQ)"; }
};


// предпосчет отрезочков
// заполняем квадратную матрицу, верхнюю половину выше диагональки
// элемент (a, b) - минимум на [a, b]
// Асимптотика по "методичке": О(N^2) - построение, О(1) - запрос, обновление имеет асимптотику О(N^2) в общем виде, отдельной функции не стоит
template<typename T>
class AllRangesRMQ : public RangeQueryStructure<T>
{
private:
    vector<vector<T>> rmq;                              // rmq[l][r] = минимум на [l, r]
    mutable int64_t build_ops = 0;
    mutable int64_t query_ops = 0;

public:
    void build(const vector<T>& arr) override
    {
        int n = arr.size();
        rmq.assign(n, vector<T>(n));

        for (int l = 0; l < n; l++)
        {
            T cur_min = arr[l];
            rmq[l][l] = cur_min;                         // заполняем диагональку таблички

            for (int r = l + 1; r < n; r++)              // считаем минимумы каждого отрезочка как умеем(ого а мы умеем???)
            {
                cur_min = min(cur_min, arr[r]);
                rmq[l][r] = cur_min;
                build_ops++;                             // доминирующая(снова морально давим на O-большое) операция: один вызов min на итерацию
            }
        }
    }

    T query(int l, int r) override                      // долгий счет - легкий запрос
    {
        if (l < 0) l = 0;
        if (r >= (int)rmq.size()) r = rmq.size() - 1;
        if (l > r) swap(l, r);

        query_ops++;                                    // обращаем внимание на обращение к памяти
        return rmq[l][r];
    }

    int64_t getBuildOps() const override { return build_ops; }
    int64_t getQueryOps() const override { return query_ops; }

    string getName() const override { return "All Ranges RMQ (O(n²))"; }
};


// корневая декомпозиция
// и для RMQ и для RSQ, поэтому используем лямбда функции и нейтральные элементы
// делим массив на блоки размером примерно sqrt(N), в каждом считаем сумму/мин элемент
// получается если в [l, r] попало несколько блоков - мы берем делаем одну операцию вместо sqrt(N)
// Асимптотика по "методичке": О(N) - построение, О(sqrt(N)) - запрос, О(sqrt(N)) - обновление => оно оптимальнее пока изменяется менее sqrt(N) элементов массива
template<typename T, typename Op>
class SqrtDecomposition : public RangeQueryStructure<T>
{
private:
    vector<T> arr;
    vector<T> blocks;
    int block_size = 0;
    Op op;
    T neutral;
    mutable int64_t build_ops = 0;
    mutable int64_t query_ops = 0;
    mutable int64_t update_ops = 0;

public:
    SqrtDecomposition(Op op, T neutral) : op(op), neutral(neutral) {}           // нейтральный элемент нужен для заполнения массива блоков

    void build(const vector<T>& a) override
    {
        arr = a;
        int n = arr.size();
        block_size = max(1, (int)sqrt(n));
        int num_blocks = (n + block_size - 1) / block_size;                     // n / block_size не работает, 10 и 3 даст 3 вместо 4, n+block_size не работает при 9 и 3, даст 4 вместо 3
        blocks.assign(num_blocks, neutral);

        for (int i = 0; i < n; i++)
        {
            int b = i / block_size;                                             // условный флаг нахождения в блоке, работает из-за целочисл деления
            blocks[b] = op(blocks[b], arr[i]);                                  // лямбда
            build_ops++;                                                        // доминирующая лямбда
        }
    }

    T query(int l, int r) override
    {
        if (l < 0) l = 0;
        if (r >= (int)arr.size()) r = (int)arr.size() - 1;
        if (l > r) swap(l, r);

        T res = neutral;
        int i = l;

        while (i <= r && i % block_size != 0)               // l может лежать в середине какого блока номер k
        {
            res = op(res, arr[i]);
            query_ops++;                                    // доминирующая лямбда
            i++;
        }

        while (i + block_size - 1 <= r)                     // самый сок алгоритма, позволяющий проходить с шагом block_size ~ aqrt(N), вплоть до последнего блока, полностью лежащего в отрезке
        {
            res = op(res, blocks[i / block_size]);
            query_ops++;                                    // доминирующая лямбда
            i += block_size;
        }

        while (i <= r)                                      // r может лежать в середине блока номер p
        {
            res = op(res, arr[i]);
            query_ops++;                                    // доминирующая лямбда
            i++;
        }

        return res;
    }

    void update(int idx, T val) override                    // меняем 1 значение в массиве изначальном, показываем оптимальный код обновление полей класса
    {
        if (idx < 0 || idx >= (int)arr.size()) return;

        arr[idx] = val;

        int b = idx / block_size;
        T block_res = neutral;
        int start = b * block_size;
        int end = min(start + block_size, (int)arr.size());         // просто заново пересчитали затронутый блок. его размер около sqrt(N), это и есть асимптотика

        for (int i = start; i < end; i++)
        {
            block_res = op(block_res, arr[i]);
            update_ops++;                                               // доминирующая операция: op при пересчёте блока
        }
        blocks[b] = block_res;
    }

    int64_t getBuildOps() const override { return build_ops; }
    int64_t getQueryOps() const override { return query_ops; }
    int64_t getUpdateOps() const override { return update_ops; }

    string getName() const override { return "Sqrt Decomposition"; }
};


// дерево отрезков
// рекурсивно "строим дерево" внутри массива, для элемента x дети это 2x и 2x+1
// op чтобы универсально считать и минимум и сумму, в корне - знач для всего массива
// потом мы берем диапазон [l, r] и пусть это не весь массив. Условно массив у нас на 10 элементов от 1 до 10 и RSQ, а [l, r] = [6, 8]
// 1. tm = 4.
// 1.1. Левая рекурсия (2, 0, 4, 6, 4) - нейтральный элемент, для суммы 0
// 1.2. Правая рекурсия (3, 5, 9, 6, 8) - снова полностью включение
// 2. tm = 5 + 4/2 = 7
// 2.1. Левая рекурсия от правой рекурсии (6, 5, 7, 6, 7) - все еще включение и так далее
// для каждого уровня дерева у нас может быть ток 2 узла, ведущих вглубь, тк у нас только 2 границы. У остальных [tl, tr] лежит в [l, r], а значит мы просто возваращем tree[node]
// значит, асимптотика запроса не O(N), a O(log2(N))
// О(N) - построение, для N листьев около 2N-1 внутренних узлов
// О(log2(N)) - обновление, т.к. по сути это чистый путь от нужно листа к корню
template<typename T, typename Op>
class SegmentTree : public RangeQueryStructure<T>
{
private:
    vector<T> tree;
    int n = 0;
    Op op;
    T neutral;
    mutable int64_t build_ops = 0;
    mutable int64_t query_ops = 0;
    mutable int64_t update_ops = 0;

    void build_rec(int node, int tl, int tr, const vector<T>& arr)          // номер узла, правый и левый край, массив значений изначальный
    {
        if (tl == tr)
        {
            tree[node] = arr[tl];                                           // левый правый края совпали - очев же что лист
            return;
        }
        int tm = tl + (tr - tl) / 2;                                        // середка между правым да левым краем
        build_rec(2 * node, tl, tm, arr);                                // 1 расплодится в 2 и 3, 2 в 4 и 5, 3 в 6 и 7, ноуды точно норм
        build_rec(2 * node + 1, tm + 1, tr, arr);                    // вызываем относительно правой левый половин
        tree[node] = op(tree[2 * node], tree[2 * node + 1]);                // схема бинарного дерева в массиве. прямые потомки x всегда 2*x и 2*x + 1
        build_ops++;                                                        // доминирующе рождение детей, тут расположено чтобы точно несколько раз не посчиталось, все рекусии прошли уже
    }

    T query_rec(int node, int tl, int tr, int l, int r)
    {
        if (l > r) return neutral;

        if (l == tl && r == tr) return tree[node];

        int tm = tl + (tr - tl) / 2;
        T left_res = query_rec(2 * node, tl, tm, l, min(r, tm));
        T right_res = query_rec(2 * node + 1, tm + 1, tr, max(l, tm + 1), r);       //

        query_ops++; // доминирующая операция: op при объединении результатов
        return op(left_res, right_res);
    }

    void update_rec(int node, int tl, int tr, int pos, T val)
    {
        if (tl == tr)                                                   // рекурсией идем идем идем до нужной вершины, заменили ее и наконец выходим из рекурсией, делая op с её родителями
        {
            tree[node] = val;
            return;
        }
        int tm = tl + (tr - tl) / 2;
        if (pos <= tm)                                                  // рекурсия выборочная, тк нам нужно поменять ток предков arr[pos]
        {
            update_rec(2 * node, tl, tm, pos, val);
        } else {
            update_rec(2 * node + 1, tm + 1, tr, pos, val);
        }
        tree[node] = op(tree[2 * node], tree[2 * node + 1]);
        update_ops++;                                                   // доминирующая лямбда при пересчете
    }

public:
    SegmentTree(Op op, T neutral) : op(op), neutral(neutral) {}

    void build(const vector<T>& arr) override
    {
        n = arr.size();
        tree.assign(4 * n, neutral);                                // должно хватить
        if (n > 0)
        {
            build_rec(1, 0, n - 1, arr);
        }
    }

    T query(int l, int r) override
    {
        if (n == 0) return neutral;
        if (l < 0) l = 0;
        if (r >= n) r = n - 1;
        if (l > r) swap(l, r);
        return query_rec(1, 0, n - 1, l, r);
    }

    void update(int idx, T val) override
    {
        if (n == 0 || idx < 0 || idx >= n) return;
        update_rec(1, 0, n - 1, idx, val);
    }

    int64_t getBuildOps() const override { return build_ops; }
    int64_t getQueryOps() const override { return query_ops; }
    int64_t getUpdateOps() const override { return update_ops; }

    string getName() const override { return "Segment Tree"; }
};


//функции в помощь
// Специализация для int
vector<int> generateRandomArray(int n, int min_val, int max_val)                // рандомный инт массив нужного размера между нужными числами
{
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dist(min_val, max_val);
    vector<int> arr(n);
    for (int i = 0; i < n; i++) {
        arr[i] = dist(gen);
    }
    return arr;
}

// Специализация для double
vector<double> generateRandomArray(int n, double min_val, double max_val)           // рандомный доубле массив нужного размера между нужными числами
{
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dist(min_val, max_val);
    vector<double> arr(n);
    for (int i = 0; i < n; i++) {
        arr[i] = dist(gen);
    }
    return arr;
}

// Специализация для int
vector<vector<int>> generateRandomMatrix(int n, int m, int min_val, int max_val)        // рандомная инт матрица нужного размера между нужными числами
{
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dist(min_val, max_val);
    vector<vector<int>> matrix(n, vector<int>(m));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            matrix[i][j] = dist(gen);
        }
    }
    return matrix;
}

// Специализация для double
vector<vector<double>> generateRandomMatrix(int n, int m, double min_val, double max_val)       // рандомная доубле матрица нужного размера между нужными числами
{
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dist(min_val, max_val);
    vector<vector<double>> matrix(n, vector<double>(m));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            matrix[i][j] = dist(gen);
        }
    }
    return matrix;
}



int main()
{
    // csw текстовый формат таблиц, который удобно обрабатывать
    ofstream csv("lab3_results.csv", ios::binary);
    csv << "\xEF\xBB\xBF";
    // Заголовки столбцов
    csv << "Размер N;"
        << "Построение 1D Префиксных Сумм;Запрос 1D Префиксных Сумм;"
        << "Построение 2D Префиксных Сумм;Запрос 2D Префиксных Сумм;"
        << "Построение Предпросчета RMQ;Запрос Предпросчета RMQ;"
        << "Построение Корневой Декомпозиции RSQ;Запрос Корневой Декомпозиции RSQ;"
        << "Построение Корневой Декомпозиции RMQ;Запрос Корневой Декомпозиции RMQ;"
        << "Построение Дерева Отрезков;Запрос Дерева Отрезков;Обновление Дерева Отрезков\n";

    vector<int> sizes = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200};          // размеры
    int num_arrays = 100;                                                   // кол-во разл массивов размера N
    int num_queries_per_array = 15;                                         // кол-во разных рандомных запросов для конкретного массива размера N

    // лямбды и константы
    auto sum_op_int = [](int a, int b) { return a + b; };
    auto min_op_int = [](int a, int b) { return min(a, b); };
    const int INF_INT = 2e9;

    auto sum_op_double = [](double a, double b) { return a + b; };
    auto min_op_double = [](double a, double b) { return min(a, b); };
    const double INF_DOUBLE = 2e9;

    for (int n : sizes)         // по массиву размеров
    {
        cout << "Testing N = " << n << "..." << endl;

        int64_t total_pref1d_build = 0, total_pref1d_query = 0;
        int64_t total_pref2d_build = 0, total_pref2d_query = 0;
        int64_t total_rmq_all_build = 0, total_rmq_all_query = 0;
        int64_t total_sqrt_rsq_build = 0, total_sqrt_rsq_query = 0;
        int64_t total_sqrt_rmq_build = 0, total_sqrt_rmq_query = 0;
        int64_t total_seg_build = 0, total_seg_query = 0, total_seg_update = 0;
        // завели счетчики на эту итерацию массивов размера N

        for (int arr_idx = 0; arr_idx < num_arrays; arr_idx++)          // проходка по 100 массива. четные - инт, нечетные - доубл
        {
            if (arr_idx % 2 == 0)
            {
                vector<int> arr = generateRandomArray(n, -1000, 1000);

                PrefixSum1D<int> pref1d;
                pref1d.build(arr);
                total_pref1d_build += pref1d.getBuildOps();
                for (int q = 0; q < num_queries_per_array; q++)
                {
                    int64_t before = pref1d.getQueryOps();
                    int l = rand() % n;
                    int r = rand() % n;
                    if (l > r) swap(l, r);
                    pref1d.query(l, r);
                    total_pref1d_query += pref1d.getQueryOps() - before;
                }

                vector<vector<int>> mat = generateRandomMatrix(n, n, -1000, 1000);
                PrefixSum2D<int> pref2d;
                pref2d.build2D(mat);
                total_pref2d_build += pref2d.getBuildOps();
                for (int q = 0; q < num_queries_per_array; q++)
                {
                    int64_t before = pref2d.getQueryOps();
                    int x1 = rand() % n, y1 = rand() % n;
                    int x2 = rand() % n, y2 = rand() % n;
                    if (x1 > x2) swap(x1, x2);
                    if (y1 > y2) swap(y1, y2);
                    pref2d.query2D(x1, y1, x2, y2);
                    total_pref2d_query += pref2d.getQueryOps() - before;
                }

                AllRangesRMQ<int> rmq_all;
                rmq_all.build(arr);
                total_rmq_all_build += rmq_all.getBuildOps();
                for (int q = 0; q < num_queries_per_array; q++)
                {
                    int64_t before = rmq_all.getQueryOps();
                    int l = rand() % n;
                    int r = rand() % n;
                    if (l > r) swap(l, r);
                    rmq_all.query(l, r);
                    total_rmq_all_query += rmq_all.getQueryOps() - before;
                }

                SqrtDecomposition<int, decltype(sum_op_int)> sqrt_rsq(sum_op_int, 0);
                sqrt_rsq.build(arr);
                total_sqrt_rsq_build += sqrt_rsq.getBuildOps();
                for (int q = 0; q < num_queries_per_array; q++)
                {
                    int64_t before = sqrt_rsq.getQueryOps();
                    int l = rand() % n;
                    int r = rand() % n;
                    if (l > r) swap(l, r);
                    sqrt_rsq.query(l, r);
                    total_sqrt_rsq_query += sqrt_rsq.getQueryOps() - before;
                }

                SqrtDecomposition<int, decltype(min_op_int)> sqrt_rmq(min_op_int, INF_INT);
                sqrt_rmq.build(arr);
                total_sqrt_rmq_build += sqrt_rmq.getBuildOps();
                for (int q = 0; q < num_queries_per_array; q++)
                {
                    int64_t before = sqrt_rmq.getQueryOps();
                    int l = rand() % n;
                    int r = rand() % n;
                    if (l > r) swap(l, r);
                    sqrt_rmq.query(l, r);
                    total_sqrt_rmq_query += sqrt_rmq.getQueryOps() - before;
                }

                SegmentTree<int, decltype(sum_op_int)> seg_rsq(sum_op_int, 0);
                seg_rsq.build(arr);
                total_seg_build += seg_rsq.getBuildOps();
                for (int q = 0; q < num_queries_per_array; q++)
                {
                    int64_t before = seg_rsq.getQueryOps();
                    int l = rand() % n;
                    int r = rand() % n;
                    if (l > r) swap(l, r);
                    seg_rsq.query(l, r);
                    total_seg_query += seg_rsq.getQueryOps() - before;
                }
                for (int q = 0; q < num_queries_per_array; q++)
                {
                    int64_t before = seg_rsq.getUpdateOps();
                    int idx = rand() % n;
                    int val = rand() % 2000 - 1000;
                    seg_rsq.update(idx, val);
                    total_seg_update += seg_rsq.getUpdateOps() - before;
                }

            } else {
                vector<double> arr = generateRandomArray(n, -1000.0, 1000.0);

                PrefixSum1D<double> pref1d;
                pref1d.build(arr);
                total_pref1d_build += pref1d.getBuildOps();
                for (int q = 0; q < num_queries_per_array; q++) {
                    int64_t before = pref1d.getQueryOps();
                    int l = rand() % n;
                    int r = rand() % n;
                    if (l > r) swap(l, r);
                    pref1d.query(l, r);
                    total_pref1d_query += pref1d.getQueryOps() - before;
                }

                // 2D Prefix Sum
                vector<vector<double>> mat = generateRandomMatrix(n, n, -1000.0, 1000.0);
                PrefixSum2D<double> pref2d;
                pref2d.build2D(mat);
                total_pref2d_build += pref2d.getBuildOps();
                for (int q = 0; q < num_queries_per_array; q++) {
                    int64_t before = pref2d.getQueryOps();
                    int x1 = rand() % n, y1 = rand() % n;
                    int x2 = rand() % n, y2 = rand() % n;
                    if (x1 > x2) swap(x1, x2);
                    if (y1 > y2) swap(y1, y2);
                    pref2d.query2D(x1, y1, x2, y2);
                    total_pref2d_query += pref2d.getQueryOps() - before;
                }

                AllRangesRMQ<double> rmq_all;
                rmq_all.build(arr);
                total_rmq_all_build += rmq_all.getBuildOps();
                for (int q = 0; q < num_queries_per_array; q++) {
                    int64_t before = rmq_all.getQueryOps();
                    int l = rand() % n;
                    int r = rand() % n;
                    if (l > r) swap(l, r);
                    rmq_all.query(l, r);
                    total_rmq_all_query += rmq_all.getQueryOps() - before;
                }

                SqrtDecomposition<double, decltype(sum_op_double)> sqrt_rsq(sum_op_double, 0.0);
                sqrt_rsq.build(arr);
                total_sqrt_rsq_build += sqrt_rsq.getBuildOps();
                for (int q = 0; q < num_queries_per_array; q++) {
                    int64_t before = sqrt_rsq.getQueryOps();
                    int l = rand() % n;
                    int r = rand() % n;
                    if (l > r) swap(l, r);
                    sqrt_rsq.query(l, r);
                    total_sqrt_rsq_query += sqrt_rsq.getQueryOps() - before;
                }

                SqrtDecomposition<double, decltype(min_op_double)> sqrt_rmq(min_op_double, INF_DOUBLE);
                sqrt_rmq.build(arr);
                total_sqrt_rmq_build += sqrt_rmq.getBuildOps();
                for (int q = 0; q < num_queries_per_array; q++) {
                    int64_t before = sqrt_rmq.getQueryOps();
                    int l = rand() % n;
                    int r = rand() % n;
                    if (l > r) swap(l, r);
                    sqrt_rmq.query(l, r);
                    total_sqrt_rmq_query += sqrt_rmq.getQueryOps() - before;
                }

                SegmentTree<double, decltype(sum_op_double)> seg_rsq(sum_op_double, 0.0);
                seg_rsq.build(arr);
                total_seg_build += seg_rsq.getBuildOps();
                for (int q = 0; q < num_queries_per_array; q++) {
                    int64_t before = seg_rsq.getQueryOps();
                    int l = rand() % n;
                    int r = rand() % n;
                    if (l > r) swap(l, r);
                    seg_rsq.query(l, r);
                    total_seg_query += seg_rsq.getQueryOps() - before;
                }
                for (int q = 0; q < num_queries_per_array; q++) {
                    int64_t before = seg_rsq.getUpdateOps();
                    int idx = rand() % n;
                    double val = (rand() % 2000 - 1000) / 10.0;
                    seg_rsq.update(idx, val);
                    total_seg_update += seg_rsq.getUpdateOps() - before;
                }
            }
        }

        double total_queries = num_arrays * num_queries_per_array;

        csv << n << ";"
        << to_csv_number((double)total_pref1d_build / num_arrays) << ";"
        << to_csv_number((double)total_pref1d_query / total_queries) << ";"
        << to_csv_number((double)total_pref2d_build / num_arrays) << ";"
        << to_csv_number((double)total_pref2d_query / total_queries) << ";"
        << to_csv_number((double)total_rmq_all_build / num_arrays) << ";"
        << to_csv_number((double)total_rmq_all_query / total_queries) << ";"
        << to_csv_number((double)total_sqrt_rsq_build / num_arrays) << ";"
        << to_csv_number((double)total_sqrt_rsq_query / total_queries) << ";"
        << to_csv_number((double)total_sqrt_rmq_build / num_arrays) << ";"
        << to_csv_number((double)total_sqrt_rmq_query / total_queries) << ";"
        << to_csv_number((double)total_seg_build / num_arrays) << ";"
        << to_csv_number((double)total_seg_query / total_queries) << ";"
        << to_csv_number((double)total_seg_update / total_queries) << "\n";
    }

    // Добавляем строку с ожидаемой асимптотикой
    csv << "Ожидаемая сложность;"
        << "O(N);O(1);"
        << "O(N^2);O(1);"
        << "O(N^2);O(1);"
        << "O(N);O(sqrt(N));"
        << "O(N);O(sqrt(N));"
        << "O(N);O(logN);O(logN)\n";

    csv.close();
    cout << "\nResults saved to lab3_results.csv" << endl;

    return 0;
}
#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <fstream>
#include <algorithm>

using namespace std;

//интерфейс для построения чего-то полезного
template<typename T>
class RangeQueryStructure {
public:
    virtual ~RangeQueryStructure() = default;

    //построение структуры из массива
    virtual void build(const vector<T>& arr) = 0;

    //запросик на отрезке
    virtual T query(int l, int r) = 0;

    //геттеры для счётчиков операций
    virtual int64_t getBuildOps() const = 0;
    virtual int64_t getQueryOps() const = 0;

    //имя структуры (для вывода)
    virtual string getName() const = 0;
};

//одномерные префиксные суммы
template<typename T>
class PrefixSum1D : public RangeQueryStructure<T> {
private:
    vector<T> prefix;        // префиксные суммы
    mutable int64_t build_ops = 0;
    mutable int64_t query_ops = 0;

public:
    void build(const vector<T>& arr) override {
        build_ops = 0;
        int n = arr.size();
        prefix.assign(n + 1, 0);

        for (int i = 0; i < n; i++) {               //считаем сумму массива префикс как сумму прошлых элементов входного массива + считаем количество операций
            build_ops++; // проверка условия i < n
            prefix[i + 1] = prefix[i] + arr[i];
            build_ops += 2; // сложение и присваивание
        }
    }

    T query(int l, int r) override {
        query_ops = 0;
        if (l < 0) l = 0;
        if (r >= (int)prefix.size() - 1) r = prefix.size() - 2;
        if (l > r) return 0;

        query_ops += 2; // два доступа к prefix
        query_ops++;     // вычитание
        return prefix[r + 1] - prefix[l]; //считаем быстренько
    }

    int64_t getBuildOps() const override { return build_ops; }
    int64_t getQueryOps() const override { return query_ops; }

    string getName() const override { return "1D Prefix Sum (RSQ)"; }
};

//двумерн(ия)ые префиксные суммы ууууу
template<typename T>
class PrefixSum2D : public RangeQueryStructure<T> {
private:
    vector<vector<T>> prefix;   // 2D префиксные суммы
    int rows = 0, cols = 0;
    mutable int64_t build_ops = 0;
    mutable int64_t query_ops = 0;

public:
    void build(const vector<T>& arr) override {
        // Для 2D этот метод не используется
        throw std::logic_error("2D structure requires 2D array");
    }

    //считаем по-особенному
    void build2D(const vector<vector<T>>& matrix) {
        build_ops = 0;
        rows = matrix.size();
        if (rows == 0) return;
        cols = matrix[0].size();

        prefix.assign(rows + 1, vector<T>(cols + 1, 0));

        for (int i = 0; i < rows; i++) {
            build_ops++; // проверка i < rows
            for (int j = 0; j < cols; j++) {
                build_ops++; // проверка j < cols
                prefix[i + 1][j + 1] = matrix[i][j] //ого для эго есть какая-то сложная (нет) формула
                                     + prefix[i][j + 1]
                                     + prefix[i + 1][j]
                                     - prefix[i][j];
                build_ops += 4; //сделали много дел и устали
            }
        }
    }

    //запрос суммы в прямоугольнике [(x1,y1), (x2,y2)]
    T query2D(int x1, int y1, int x2, int y2) {
        query_ops = 0;
        if (x1 < 0) x1 = 0;
        if (y1 < 0) y1 = 0;
        if (x2 >= rows) x2 = rows - 1;
        if (y2 >= cols) y2 = cols - 1;
        if (x1 > x2 || y1 > y2) return 0;

        query_ops += 4; // 4 доступа к prefix
        query_ops += 3; // 2 сложения и 1 вычитание

        return prefix[x2 + 1][y2 + 1] //тоже очень сложная формула
             - prefix[x1][y2 + 1]
             - prefix[x2 + 1][y1]
             + prefix[x1][y1];
    }

    T query(int l, int r) override {
        throw std::logic_error("Use query2D for 2D structure");
        return 0;
    }

    int64_t getBuildOps() const override { return build_ops; }
    int64_t getQueryOps() const override { return query_ops; }

    string getName() const override { return "2D Prefix Sum (RSQ)"; }
};

//предпосчет отрезочков
template<typename T>
class AllRangesRMQ : public RangeQueryStructure<T> {
private:
    vector<vector<T>> rmq;  // rmq[l][r] = минимум на [l, r]
    mutable int64_t build_ops = 0;
    mutable int64_t query_ops = 0;

public:
    void build(const vector<T>& arr) override {
        build_ops = 0;
        int n = arr.size();
        rmq.assign(n, vector<T>(n));

        for (int l = 0; l < n; l++) {
            build_ops++; // проверка l < n
            T cur_min = arr[l];
            build_ops++; // присваивание
            rmq[l][l] = cur_min; //заполняем диагональку таблички
            build_ops++; // присваивание

            for (int r = l + 1; r < n; r++) { //считаем минимумы каждого отрезочка как умеем
                build_ops++; // проверка r < n
                cur_min = min(cur_min, arr[r]);
                build_ops += 2; // вызов min и присваивание
                rmq[l][r] = cur_min;
                build_ops++; // присваивание
            }
        }
    }

    T query(int l, int r) override { //долгий счет легкий запрос
        query_ops = 0;
        if (l < 0) l = 0;
        if (r >= (int)rmq.size()) r = rmq.size() - 1;
        if (l > r) swap(l, r);

        query_ops++; // доступ к rmq[l][r]
        return rmq[l][r];
    }

    int64_t getBuildOps() const override { return build_ops; }
    int64_t getQueryOps() const override { return query_ops; }

    string getName() const override { return "All Ranges RMQ (O(n²))"; }
};

//функции в помощь

vector<int> generateRandomArray(int n, int min_val = 1, int max_val = 1000) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dist(min_val, max_val);

    vector<int> arr(n);
    for (int i = 0; i < n; i++) {
        arr[i] = dist(gen);
    }
    return arr;
}

vector<vector<int>> generateRandomMatrix(int n, int m, int min_val = 1, int max_val = 1000) {
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


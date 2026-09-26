#ifndef __PROGTEST__

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <climits>
#include <cfloat>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <vector>
#include <set>
#include <list>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <stack>
#include <deque>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <semaphore>
#include <atomic>
#include <condition_variable>
#include "progtest_solver.h"
#include "sample_tester.h"

#endif /* __PROGTEST__ */

class CSolver {
public:
    CSolver(AProgtestSolver solver) {
        main_solver = solver;
    }

    void addOrderToComplete(ACustomer customer, AOrderList orderList) {
        orders_to_complete_after_solve.push_back(std::make_pair(customer, orderList));
    }

    AProgtestSolver main_solver;
    size_t threads_in_work = 0;
    size_t threads_completed = 0;
    std::vector<std::pair<ACustomer, AOrderList> > orders_to_complete_after_solve{};
};

using ASolver = std::shared_ptr<CSolver>;

class CWeldingCompany {
public:
    static bool usingProgtestSolver() {
        return false;
    }

    static void seqSolve(APriceList priceList,
                         COrder &order) {
        std::vector<std::vector<double> > size_to_cost(order.m_W, std::vector<double>(order.m_H, INFINITY));

        for (size_t i = 0; i < priceList->m_List.size(); i++) {
            if (priceList->m_List[i].m_H <= order.m_H && priceList->m_List[i].m_W <= order.m_W) {
                size_to_cost[priceList->m_List[i].m_W - 1][priceList->m_List[i].m_H - 1] = std::min(
                    priceList->m_List[i].m_Cost,
                    size_to_cost[priceList->m_List[i].m_W - 1][priceList->m_List[i].m_H - 1]);
            }
            if (priceList->m_List[i].m_H <= order.m_W && priceList->m_List[i].m_W <= order.m_H) {
                size_to_cost[priceList->m_List[i].m_H - 1][priceList->m_List[i].m_W - 1] = std::min(
                    priceList->m_List[i].m_Cost,
                    size_to_cost[priceList->m_List[i].m_H - 1][priceList->m_List[i].m_W - 1]);
            }
        }

        for (unsigned i = 0; i < order.m_W; ++i) {
            for (unsigned j = 0; j < order.m_H; ++j) {
                double min_cost = size_to_cost[i][j];
                for (unsigned k = 1; k <= i; k++) {
                    min_cost = std::min(
                        min_cost, size_to_cost[i - k][j] + size_to_cost[k - 1][j] + order.m_WeldingStrength * (j + 1));
                }
                for (unsigned k = 1; k <= j; k++) {
                    min_cost = std::min(
                        min_cost, size_to_cost[i][j - k] + size_to_cost[i][k - 1] + order.m_WeldingStrength * (i + 1));
                }
                size_to_cost[i][j] = min_cost;
            }
        }

        order.m_Cost = size_to_cost[order.m_W - 1][order.m_H - 1];
    }

    void addProducer(AProducer prod) {
        producers.push_back(prod);
    }

    void addCustomer(ACustomer cust) {
        customers.push_back(cust);
    }

    void addPriceList(AProducer prod,
                      APriceList priceList) {
        std::lock_guard<std::mutex> lock(pl_mtx);
        if (!producer_gave_price[prod][priceList->m_MaterialID]) {
            material_to_price_lists[priceList->m_MaterialID].push_back(priceList);
            producer_gave_price[prod][priceList->m_MaterialID]= true;
        }
        pl_con.notify_all();
    }

    void start(unsigned thrCount) {
        program_stopped = false;
        for (auto &customer: customers) {
            customer_threads.emplace_back(&CWeldingCompany::customerProcess, this, customer);
        }
        for (size_t i = 0; i < thrCount; i++) {
            worker_threads.emplace_back(&CWeldingCompany::workerProcess, this);
        }
    }

    void stop() {
        for (auto &customer: customer_threads) {
            customer.join();
        }; {
            std::unique_lock lock(order_queue_mtx);
            program_stopped = true;
            workers_con.notify_all();
        }
        for (auto &worker: worker_threads) {
            worker.join();
        }
    }

    void customerProcess(const ACustomer &customer) {
        while (true) {
            auto orderList = customer->waitForDemand();
            if (!orderList) {
                break;
            }; {
                std::lock_guard<std::mutex> lock(order_queue_mtx);
                customer_orderList_queue.push(std::make_pair(customer, orderList));
            }
            workers_con.notify_all();
        }
    }

    void workerProcess() {
        while (true) {
            std::pair<ACustomer, AOrderList> order;

            {
                std::unique_lock<std::mutex> lock(order_queue_mtx);
                workers_con.wait(lock, [&]() { return !customer_orderList_queue.empty() || program_stopped; });
                if (customer_orderList_queue.empty() && program_stopped) {
                    break;
                }
                order = customer_orderList_queue.front();
                customer_orderList_queue.pop();
            }

            auto customer = order.first; // perfect
            auto orderList = order.second; // perfect
            auto materialId = orderList->m_MaterialID; // perfect

            APriceList price_list;

                for (const auto &prod: producers) {
                    prod->sendPriceList(materialId);
                    std::unique_lock lock(pl_mtx);
                    pl_con.wait(lock, [&,prod,materialId]() { return producer_gave_price[prod][materialId]; });
                } // perfect

                price_list = std::make_shared<CPriceList>(materialId); // perfect
                for (auto &pl: material_to_price_lists[materialId]) {
                    for (auto &p: pl->m_List) {
                        price_list->m_List.push_back(p);
                    }
                }


            for (auto &_order: orderList->m_List) {
                seqSolve(price_list, _order);
            }
            customer->completed(orderList);
        }
    }

private:
    std::vector<ACustomer> customers;
    std::vector<AProducer> producers;


    std::mutex order_queue_mtx, pl_mtx, solver_mtx;

    std::condition_variable workers_con, workers_done_con, workers_done_solved_con;
    std::condition_variable pl_con;
    std::condition_variable solvers_con;

    std::map<unsigned, std::vector<APriceList> > material_to_price_lists;
    std::map<AProducer, std::map<unsigned, bool>> producer_gave_price;

    std::queue<std::pair<ACustomer, AOrderList> > customer_orderList_queue;

    std::vector<std::thread> worker_threads;
    std::vector<std::thread> customer_threads;

    std::vector<ASolver> solvers;
    ASolver solver;

    std::atomic_int16_t current_adding_index = 0;
    std::atomic_size_t current_solving_index = 0;
    std::atomic_bool program_stopped = false;
    std::atomic_int16_t workers_done_getting_orders = 0;
    std::atomic_int16_t workers_solved_orders = 0;
};

#ifndef __PROGTEST__

int main() {
    using namespace std::placeholders;
    CWeldingCompany test;

    AProducer p1 = std::make_shared<CProducerSync>(std::bind(&CWeldingCompany::addPriceList, &test, _1, _2));
    AProducerAsync p2 = std::make_shared<CProducerAsync>(std::bind(&CWeldingCompany::addPriceList, &test, _1, _2));
    test.addProducer(p1);
    test.addProducer(p2);
    test.addCustomer(std::make_shared<CCustomerTest>(2));
    p2->start();
    test.start(3);
    test.stop();
    p2->stop();
    return EXIT_SUCCESS;
}

#endif /* __PROGTEST__ */
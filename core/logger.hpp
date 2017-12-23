/*
 * Copyright (c) 2016 Shanghai Jiao Tong University.
 *     All rights reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS
 *  IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 *  express or implied.  See the License for the specific language
 *  governing permissions and limitations under the License.
 *
 * For more about this software visit:
 *
 *      http://ipads.se.sjtu.edu.cn/projects/wukong
 *
 */

#pragma once

#include <iostream>
#include <boost/serialization/unordered_map.hpp>

#include "timer.hpp"
#include "unit.hpp"

using namespace std;

class Logger {
private:
    struct req_stats {
        int query_type;
        uint64_t start_time = 0ull;
        uint64_t end_time = 0ull;

        template <typename Archive>
        void serialize(Archive &ar, const unsigned int version) {
            ar & query_type;
            ar & start_time;
            ar & end_time;
        }
    };

    unordered_map<int, req_stats> stats_map;
    float thpt = 0.0;

    uint64_t init_time = 0ull, done_time = 0ull;

    uint64_t last_time = 0ull, last_separator = 0ull;
    uint64_t last_cnt = 0;

    uint64_t interval = MSEC(50);  // 50msec

public:
    void init() {
        init_time = last_time = last_separator = timer::get_usec();
    }

    void start_record(int reqid, int type) {
        stats_map[reqid].query_type = type;
        stats_map[reqid].start_time = timer::get_usec() - init_time;
    }

    void end_record(int reqid) {
        stats_map[reqid].end_time = timer::get_usec() - init_time;
    }

    void finish() {
        done_time = timer::get_usec();
        thpt = 1000.0 * stats_map.size() / (done_time - init_time);
    }

    void set_interval(uint64_t update) { interval = update; }

    // print the throughput of a fixed interval
    void print_timely_thpt(uint64_t recv_cnt, int sid, int tid) {
        // for brevity, only print the timely thpt of a single proxy.
        if (!(sid == 0 && tid == 0)) return;

        uint64_t now = timer::get_usec();
        // periodically print timely throughput
        if ((now - last_time) > interval) {
            float cur_thpt = 1000.0 * (recv_cnt - last_cnt) / (now - last_time);
            cout << "Throughput: " << cur_thpt << "K queries/sec" << endl;
            last_time = now;
            last_cnt = recv_cnt;
        }

        // print separators per second
        if (now - last_separator > SEC(1)) {
            cout << "[" << (now - init_time) / SEC(1) << "sec]" << endl;
            last_separator = now;
        }
    }

    void merge(Logger &other) {
        for (auto s : other.stats_map)
            stats_map[s.first] = s.second;
        thpt += other.thpt;
    }

    void print_thpt() {
        cout << "Throughput: " << thpt << "K queries/sec" << endl;
    }

    void print_latency(int cnt = 1) {
        cout << "(average) latency: " << ((done_time - init_time) / cnt) << " usec" << endl;
    }

    void print_cdf() {
#if 0
        // print range throughput with certain interval
        vector<int> thpts;
        int print_interval = 200 * 1000; // 200ms

        for (auto s : stats_map) {
            int i = s.second.start_time / print_interval;
            if (thpts.size() <= i)
                thpts.resize(i + 1);
            thpts[i]++;
        }

        cout << "Range Throughput (K queries/sec)" << endl;
        for (int i = 0; i < thpts.size(); i++)
            cout << "[" << (print_interval * i) / 1000 << "ms ~ "
                 << print_interval * (i + 1) / 1000 << "ms)\t"
                 << (float)thpts[i] / (print_interval / 1000) << endl;
#endif

        // print CDF of query latency
        vector<uint64_t> cdf;
        int print_rate = stats_map.size() > 100 ? 100 : stats_map.size();

        for (auto s : stats_map)
            cdf.push_back(s.second.end_time - s.second.start_time);
        sort(cdf.begin(), cdf.end());

        cout << "CDF graph" << endl;
        int cnt = 0;
        for (int i = 0; i < cdf.size(); i++) {
            if ((i + 1) % (cdf.size() / print_rate) == 0) {
                cnt++;
                if (cnt != print_rate)
                    cout << cdf[i] << "\t";
                else
                    cout << cdf[cdf.size() - 1] << "\t";

                if (cnt % 5 == 0) cout << endl;
            }
        }
    }

    template <typename Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & stats_map;
        ar & thpt;
    }
};

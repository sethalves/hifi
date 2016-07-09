//
//  Created by Bradley Austin Davis on 2015/09/10
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_WriteLocklessRead_h
#define hifi_WriteLocklessRead_h

#include <utility>
#include <atomic>
#include <mutex>

template <typename T>
class WriteLocklessRead {
public:
    template <typename... Args>
    WriteLocklessRead(Args&&... args) : _value(std::forward<Args>(args)...) { }

    template<typename F> 
    void withWriteLock(F&& f) {
        std::unique_lock<std::mutex> lock(_mutex);
        assert(0 == _counter.load() % 2);
        _counter++;
        f(_value);
        _counter++;
        assert(0 == _counter.load() % 2);
    }

    template<typename F>
    void withConsistentRead(F&& f) const {
        uint64_t counterStart = 0, counterEnd = 1;
        // If the two values are different, someone wrote in the intervening time
        while (counterStart != counterEnd) {
            // Wait until the counter is even, indicating no write operation is ongoing
            while (0 != (counterStart = _counter.load()) % 2) {
                // If a write was detected, then acquire the lock to ensure the write is 
                // finished before checking again
                std::unique_lock<std::mutex> lock(_mutex);
            }
            // At this point we have an even starting counter
            f(_value);
            // Check the counter again.  If no write has occured during our lambda, it should
            // be clear.
            counterEnd = _counter.load();
        }
    }

private:
    mutable std::mutex _mutex;
    mutable std::atomic<uint64_t> _counter;
    T _value;
};


#endif

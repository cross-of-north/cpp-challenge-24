#pragma once

//
// time_t-keyed collection of items (referenced with smart shared pointers)
//
template < typename T > class CTimeKeyedCollection {

    public:

        typedef std::shared_ptr < T > PT;
        typedef PT t_item;
        typedef std::map < time_t, PT > t_container;

    protected:

        // items map
        t_container m_container;

        // items map access with r/w lock
        mutable std::shared_mutex m_mutex;

        // new data notification
        std::mutex m_new_data_mutex;
        std::condition_variable m_new_data_available;

        // searches an item with a key specified and adds it if it is not found provided that write access is allowed
        void DoGetItemByKey( const time_t time_key, PT & item, const bool bCanAdd ) {
            item.reset();
            if (
                auto it = m_container.find( time_key );
                it == m_container.end()
            ) {
                if ( bCanAdd ) {
                    item = std::make_shared < T >();
                    m_container.emplace( std::make_pair( time_key, item ) );
                }
            } else {
                item = it->second;
            }
        }

    public:

        // searches an item with a key specified and adds it if it is not found
        void GetItemByKey( const time_t ts, PT & item ) {
            {
                // search only in read mode
                bool bCanAdd = false;
                std::shared_lock < std::shared_mutex > lock( m_mutex );
                DoGetItemByKey( ts, item, bCanAdd );
            }
            if ( !item ) {
                // probably need to add an item, relocking in write mode
                // if an item is already added by a concurrent thread it will be found and returned
                bool bCanAdd = true;
                std::unique_lock < std::shared_mutex > lock( m_mutex );
                DoGetItemByKey( ts, item, bCanAdd );
            }
        }

        // returns oldest item data or false if the collection is empty
        bool GetOldest( PT & item, time_t & ts ) const {
            item.reset();
            ts = 0;
            std::shared_lock < std::shared_mutex > lock( m_mutex );
            auto it = m_container.begin();
            if ( it != m_container.end() ) {
                ts = it->first;
                item = it->second;
            }
            return !!item;
        }

        // returns newest item data or false if the collection is empty
        bool GetNewest( PT & item, time_t & ts ) const {
            item.reset();
            ts = 0;
            std::shared_lock < std::shared_mutex > lock( m_mutex );
            auto it = m_container.rbegin();
            if ( it != m_container.rend() ) {
                ts = it->first;
                item = it->second;
            }
            return !!item;
        }

        // returns oldest item or false if the collection is empty
        bool GetOldestItem( PT & item ) const {
            time_t ts = 0;
            return GetOldest( item, ts );
        }

        // returns oldest item timestamp or false if the collection is empty
        bool GetOldestTimestamp( time_t & ts ) const {
            PT item;
            return GetOldest( item, ts );
        }

        // returns newest item or false if the collection is empty
        bool GetNewestItem( PT & item ) const {
            time_t ts = 0;
            return GetNewest( item, ts );
        }

        // returns newest item timestamp or false if the collection is empty
        bool GetNewestTimestamp( time_t & ts ) const {
            PT item;
            return GetNewest( item, ts );
        }

        // removes the specified item from the collection
        void RemoveItem( const PT & item ) {
            std::unique_lock < std::shared_mutex > lock( m_mutex );
            for ( auto it = m_container.begin(); it != m_container.end(); ) {
                if ( it->second == item ) {
                    m_container.erase( it );
                    break;
                } else {
                    ++it;
                }
            }
        }

        // returns true if the collection is empty
        bool IsEmpty() const {
            std::shared_lock < std::shared_mutex > lock( m_mutex );
            return m_container.empty();
        }

        // removes all items with timestamps older than the time specified from the collection
        void DiscardOlderThan( const time_t min_ts ) {
            std::unique_lock < std::shared_mutex > lock( m_mutex );
            for ( auto it = m_container.begin(); it != m_container.end(); ) {
                if ( it->first < min_ts ) {
                    it = m_container.erase( it );
                } else {
                    //++it;
                    break;
                }
            }
        }

        // block for the specified time or until new data is available
        auto WaitForData( const int timeout_ms ) {
            std::unique_lock< std::mutex > lock( m_new_data_mutex );
            return m_new_data_available.wait_for( lock, std::chrono::milliseconds( timeout_ms ) );
        }

        // notify waiting thread that new data is available
        void NotifyDataAvailable() {
            std::lock_guard < std::mutex > lock( m_new_data_mutex );
            m_new_data_available.notify_one();
        }

        // explicitly add an item to the collection
        void AddItem( const time_t ts, const PT & item ) {
            std::unique_lock < std::shared_mutex > lock( m_mutex );
            m_container.emplace( std::make_pair( ts, item ) );
        }

};

#pragma once

template < typename T > class CTimeKeyedCollection {

    public:

        typedef std::shared_ptr < T > PT;
        typedef PT t_item;
        typedef std::map < time_t, PT > t_container;

    protected:

        t_container m_container;
        mutable std::shared_mutex m_mutex;
        std::mutex m_new_data_mutex;
        std::condition_variable m_new_data_available;

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

        void GetItemByKey( const time_t ts, PT & item ) {
            {
                bool bCanAdd = false;
                std::shared_lock < std::shared_mutex > lock( m_mutex );
                DoGetItemByKey( ts, item, bCanAdd );
            }
            if ( !item ) {
                // need to add?
                bool bCanAdd = true;
                std::unique_lock < std::shared_mutex > lock( m_mutex );
                DoGetItemByKey( ts, item, bCanAdd );
            }
        }

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

        bool GetNewestItem( PT & item ) const {
            time_t ts = 0;
            return GetNewest( item, ts );
        }

        bool GetNewestTimestamp( time_t & ts ) const {
            PT item;
            return GetNewest( item, ts );
        }

        bool GetOldestItem( PT & item ) const {
            time_t ts = 0;
            return GetOldest( item, ts );
        }

        bool GetOldestTimestamp( time_t & ts ) const {
            PT item;
            return GetOldest( item, ts );
        }

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

        bool IsEmpty() const {
            std::shared_lock < std::shared_mutex > lock( m_mutex );
            return m_container.empty();
        }

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

        auto WaitForData( const int timeout_ms ) {
            std::unique_lock< std::mutex > lock( m_new_data_mutex );
            return m_new_data_available.wait_for( lock, std::chrono::milliseconds( timeout_ms ) );
        }

        void NotifyDataAvailable() {
            std::lock_guard < std::mutex > lock( m_new_data_mutex );
            m_new_data_available.notify_one();
        }

        void AddItem( const time_t ts, const PT & item ) {
            std::unique_lock < std::shared_mutex > lock( m_mutex );
            m_container.emplace( std::make_pair( ts, item ) );
        }

};

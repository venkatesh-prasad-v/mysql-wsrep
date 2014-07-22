/* Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; version 2 of the
   License.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
   02110-1301 USA */

#include "sql_class.h"
#include "rpl_context.h"

Session_consistency_gtids_ctx::Session_consistency_gtids_ctx() : m_sid_map(NULL),
        m_gtid_set(NULL), m_listener(NULL) { }

Session_consistency_gtids_ctx::~Session_consistency_gtids_ctx()
{
  if (m_gtid_set)
  {
    delete m_gtid_set;
    m_gtid_set= NULL;
  }

  if (m_sid_map)
  {
    delete m_sid_map;
    m_sid_map= NULL;
  }
}

inline bool Session_consistency_gtids_ctx::shall_collect(const THD* thd)
{
  return  gtid_mode == GTID_MODE_ON &&
          /* if there is no listener/tracker, then there is no reason to collect */
          m_listener != NULL &&
          /* ROLLBACK statements may end up calling trans_commit_stmt */
          thd->lex->sql_command != SQLCOM_ROLLBACK &&
          thd->lex->sql_command != SQLCOM_ROLLBACK_TO_SAVEPOINT;
}

bool Session_consistency_gtids_ctx::notify_after_transaction_commit(const THD* thd)
{
  DBUG_ENTER("Rpl_consistency_ctx::notify_after_transaction_commit");
  DBUG_ASSERT(thd);
  bool res= false;

  if (!shall_collect(thd))
    DBUG_RETURN(res);

  if (thd->variables.session_track_gtids == ALL_GTIDS)
  {
    /*
     If one is configured to read all writes, we always collect
     the GTID_EXECUTED.

     NOTE: in the future optimize to collect deltas instead maybe.
    */
    global_sid_lock->rdlock();
    res= m_gtid_set->add_gtid_set(gtid_state->get_executed_gtids()) != RETURN_STATUS_OK;
    global_sid_lock->unlock();

    if (!res)
      notify_ctx_change_listener();
  }

  DBUG_RETURN(res);
}

bool Session_consistency_gtids_ctx::notify_after_gtid_executed_update(const THD *thd)
{
  DBUG_ENTER("Rpl_consistency_ctx::notify_after_gtid_executed_update");
  DBUG_ASSERT(thd);
  DBUG_ASSERT(gtid_mode == GTID_MODE_ON);
  bool res= false;

  if (!shall_collect(thd))
    DBUG_RETURN(res);

  if (thd->variables.session_track_gtids == OWN_GTID)
  {
    bool added= false;
    const Gtid& gtid= thd->owned_gtid;
    if (gtid.sidno == -1) // we need to add thd->owned_gtid_set
    {
      /* Caller must only call this function if the set was not empty. */
      DBUG_ASSERT(!thd->owned_gtid_set.is_empty());
#ifdef HAVE_GTID_NEXT_LIST
      res= m_gtid_set->add_gtid_set(&thd->owned_gtid_set) != RETURN_STATUS_OK;
      if (!res)
        added= true;
#else
      DBUG_ASSERT(0);
#endif
    }
    else if (gtid.sidno > 0) // only one gtid
    {
      DBUG_ASSERT(!m_gtid_set->contains_gtid(gtid.sidno, gtid.gno));
      res= (m_gtid_set->ensure_sidno(gtid.sidno) != RETURN_STATUS_OK ||
            m_gtid_set->_add_gtid(gtid.sidno, gtid.gno) != RETURN_STATUS_OK);

      if (!res)
        added= true;
    }

    if (added)
      notify_ctx_change_listener();
  }
  DBUG_RETURN(res);
}

bool Session_consistency_gtids_ctx::notify_after_response_packet(const THD *thd)
{
  int res= false;
  DBUG_ENTER("Rpl_consistency_ctx::notify_after_response_packet");
  if (gtid_mode != GTID_MODE_ON)
    DBUG_RETURN(res);

  if (m_gtid_set && !m_gtid_set->is_empty())
    m_gtid_set->clear();

  DBUG_RETURN(res);
}

void
Session_consistency_gtids_ctx::register_ctx_change_listener(
              Session_consistency_gtids_ctx::Ctx_change_listener* listener)
{
  DBUG_ASSERT(m_listener == NULL || m_listener == listener);
  if (m_listener == NULL)
  {
    DBUG_ASSERT(m_sid_map == NULL && m_gtid_set == NULL);
    m_listener= listener;
    m_sid_map= new Sid_map(NULL);
    m_gtid_set= new Gtid_set(m_sid_map);
  }
}

void Session_consistency_gtids_ctx::unregister_ctx_change_listener(
             Session_consistency_gtids_ctx::Ctx_change_listener* listener)
{
  DBUG_ASSERT(m_listener == listener || m_listener == NULL);

  if (m_gtid_set)
    delete m_gtid_set;

  if (m_sid_map)
    delete m_sid_map;

  m_listener= NULL;
  m_gtid_set= NULL;
  m_sid_map= NULL;
}
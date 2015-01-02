drop table if exists users_with_comments;
create table users_with_comments
select
    u.*,
    count(c.comment_id) as 'comment_count'
from
    wp_users u
right outer join
    wp_comments c on
        c.user_id = u.id
where
    c.user_id <> 0
group by
    u.user_login
union
select
    u.*,
    0 as 'comment_count'
from
    wp_users u
where
    u.user_login = 'admin';

drop table if exists users_with_no_comments;
create table users_with_no_comments
select
    u.*
from
    wp_users u
where
    id not in (select id from users_with_comments);

drop table if exists users_with_matching_login_and_display_name;
create table users_with_matching_login_and_display_name
select
    u.*
from
    wp_users u
where
    u.user_login = u.display_name;

drop table if exists users_with_empty_url;
create table users_with_empty_url
select
    u.*
from
    wp_users u
where
    user_url = '';

drop table if exists users_summary;
create table users_summary
select
    u.id,
    u.user_login,
    coalesce((select 1 from users_with_empty_url where id = u.id), 0) as 'users_with_empty_url',
    coalesce((select 1 from users_with_no_comments where id = u.id), 0) as 'users_with_no_comments',
    coalesce((select 1 from users_with_matching_login_and_display_name where id = u.id), 0) as 'users_with_matching_login_and_display_name'
from
    wp_users u;

drop table if exists spam_users;
create table spam_users
select
    u.*
from
    users_summary s
inner join
    wp_users u on
        u.id = s.id
where
    s.users_with_empty_url = 1 and
    s.users_with_no_comments = 1 and
    s.users_with_matching_login_and_display_name = 1;

drop table if exists spam_usermeta;
create table spam_usermeta
select
    m.*
from
    wp_usermeta m
inner join
    spam_users s on
        m.user_id = s.id;

delete
    u,
    m
from
    wp_users as u,
    wp_usermeta as m,
    spam_users as s
where
    u.id = s.id and
    m.user_id = s.id;

drop table users_summary;
drop table users_with_empty_url;
drop table users_with_comments;
drop table users_with_no_comments;
drop table users_with_matching_login_and_display_name;


-- vim:set ts=8 sw=4 sts=4 tw=78 et:

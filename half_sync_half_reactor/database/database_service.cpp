#include <mysql/mysql.h>
#include <iostream>

bool TestQueryDatabase(MYSQL& sql, const std::string& user, const std::string& password, 
    const std::string& database);

bool TestInsertDatabase(MYSQL& mysql, const std::string& user, const std::string& password, 
    const std::string& database);

bool TestDeleteDatabase(MYSQL& mysql, const std::string& user, const std::string& password, 
    const std::string& database);

int main(int argc, const char* argv[]) {

  std::string user = "root";
  std::string password = "5201314Cong";
  std::string host = "localhost";
  std::string database = "custom_test_database";

  MYSQL mysql;

  if (!(mysql_init(&mysql))) 
  {
    printf("mysql_init():%s\n", mysql_error(&mysql));
    return -1;
  }

  if (!(mysql_real_connect(&mysql, host.c_str(), user.c_str(), password.c_str(), 
          database.c_str(), 0, nullptr, 0)))
  {
    printf("mysql_real_connect(): %s\n", mysql_error(&mysql));
    return -1;
  }

  printf("Connected MySQL successful! \n");

  // Do something
  
  /*Insert*/
  TestInsertDatabase(mysql, user, password, database);

  /*Query*/
  TestQueryDatabase(mysql, user, password, database);

  /*Delete*/
  TestDeleteDatabase(mysql, user, password, database);

  printf("after delete \n\n\n");

  /*Query*/
  TestQueryDatabase(mysql, user, password, database);

  mysql_close(&mysql);

  return 0;
}

bool TestInsertDatabase(MYSQL& mysql, const std::string& user, const std::string& password, 
    const std::string& database)
{
  std::string option = "set names utf8";
  mysql_real_query(&mysql, option.c_str(), option.size());

  std::string query_str = "insert into msg (title, name, content) values(\"mysql有没有好用的库 \", \"petter\", \"找一个github上好用的库\")";
  int ret = mysql_real_query(&mysql, query_str.c_str(), query_str.size());
  if (0 != ret) 
  {
    printf("mysql_real_query(): %s\n", mysql_error(&mysql));
    return false;
  }
  return true;
}

bool TestDeleteDatabase(MYSQL& mysql, const std::string& user, const std::string& password, 
    const std::string& database)
{

  std::string option = "set names utf8";
  mysql_real_query(&mysql, option.c_str(), option.size());

  std::string query_str = "delete from msg where id = 3";
  int ret = mysql_real_query(&mysql, query_str.c_str(), query_str.size());
  if (0 != ret) 
  {
    printf("mysql_real_query(): %s\n", mysql_error(&mysql));
    return false;
  }

  return true;
}

bool TestQueryDatabase(MYSQL& sql, const std::string& user, const std::string& password, 
    const std::string& database) 
{
  MYSQL_RES* result;
  MYSQL_ROW sql_row;

  int ret = 0;

  std::string option = "set names utf8";
  mysql_real_query(&sql, option.c_str(), option.size());

  std::string sql_str = "select * from msg";
  ret = mysql_real_query(&sql, sql_str.c_str(), sql_str.size());

  if (ret == 0)
  {
    result = mysql_store_result(&sql);
    if (result) 
    {
      int fields = mysql_num_fields(result);
      int rows = mysql_num_rows(result);
      printf("有多少行数据: %d\n", rows);
      printf("每行数据有多少列: %d\n", fields);
      printf("desc table msg\n");
      while ((sql_row = mysql_fetch_row(result)))
      {
        printf("get message:\n");
        for (int i = 0; i < fields; ++i)
        {
          printf("%s ", sql_row[i]);
        }
        printf("\n");
      }
    }
    else
    {
      std::cout << "mysql_store_result is failured" << std::endl;
    }
  }
  else 
  {
    std::cout << "mysql_query is failured" << std::endl;
  }

  if (ret) 
  {
    mysql_free_result(result);
  }

  return 0;
}

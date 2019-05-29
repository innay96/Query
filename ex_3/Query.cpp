#include "Query.h"
#include "TextQuery.h"
#include <memory>
#include <set>
#include <algorithm>
#include <iostream>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <regex>
using namespace std;
////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<QueryBase> QueryBase::factory(const string& s)
{
 //  if(s == "smart") return std::shared_ptr<QueryBase>(new WordQuery("smart")); ---- example
  regex regex_and("^\\s*([\\w']+)\\s+AND\\s+([\\w']+)\\s*$");
  regex regex_or("^\\s*([\\w']+)\\s+OR\\s+([\\w']+)\\s*$");
  regex regex_n("^\\s*([\\w']+)\\s+([\\d]+)\\s+([\\w']+)\\s*$");
  regex regex_not("^\\s*NOT\\s+([\\w']+)\\s*$");
  regex regex_word("^\\s*([\\w']+)\\s*$");

  //WORD
  if(regex_match(s, regex_word)) 
      return std::shared_ptr<QueryBase>(new WordQuery(s));

  //AND
    if(regex_match(s, regex_and)){
      auto words_begin = std::sregex_iterator(s.begin(), s.end(), regex_and);
      auto words_end = std::sregex_iterator();
      if (std::distance(words_begin, words_end) > 0)
        return std::shared_ptr<QueryBase>(new AndQuery((*words_begin)[1].str(), (*words_begin)[2].str()));
    }

  //OR
  auto words_begin = std::sregex_iterator(s.begin(), s.end(), regex_or);
  auto words_end = std::sregex_iterator();
    if (std::distance(words_begin, words_end) > 0)
      return std::shared_ptr<QueryBase>(new OrQuery((*words_begin)[1].str(), (*words_begin)[2].str()));
    
  //n
  if(regex_match(s, regex_n)){
    istringstream iss(s);
		vector<string> res((istream_iterator<string>(iss)),istream_iterator<string>());
		return shared_ptr<QueryBase>(new NQuery(res[0], res[2],stoi(res[1])));
  }

  //NOT
  auto wordsRegex = sregex_iterator(s.begin(), s.end(), regex_not);
    if(s == wordsRegex -> str()){
       return std::shared_ptr<QueryBase>(new NotQuery((*wordsRegex)[1].str()));
    }

  throw invalid_argument("Unrecognized search");
  
}
////////////////////////////////////////////////////////////////////////////////
QueryResult NotQuery::eval(const TextQuery &text) const
{
  QueryResult result = text.query(query_word);
  auto ret_lines = std::make_shared<std::set<line_no>>();
  auto beg = result.begin(), end = result.end();
  auto sz = result.get_file()->size();
  
  for (size_t n = 0; n != sz; ++n)
  {
    if (beg==end || *beg != n)
		ret_lines->insert(n);
    else if (beg != end)
		++beg;
  }
  return QueryResult(rep(), ret_lines, result.get_file());
}

QueryResult AndQuery::eval (const TextQuery& text) const
{
  QueryResult left_result = text.query(left_query);
  QueryResult right_result = text.query(right_query);
  
  auto ret_lines = std::make_shared<std::set<line_no>>();
  std::set_intersection(left_result.begin(), left_result.end(),
      right_result.begin(), right_result.end(), 
      std::inserter(*ret_lines, ret_lines->begin()));

  return QueryResult(rep(), ret_lines, left_result.get_file());
}

QueryResult OrQuery::eval(const TextQuery &text) const
{
  QueryResult left_result = text.query(left_query);
  QueryResult right_result = text.query(right_query);
  
  auto ret_lines = 
      std::make_shared<std::set<line_no>>(left_result.begin(), left_result.end());

  ret_lines->insert(right_result.begin(), right_result.end());

  return QueryResult(rep(), ret_lines, left_result.get_file());
}
/////////////////////////////////////////////////////////
QueryResult NQuery::eval(const TextQuery &text) const
{
  QueryResult res = AndQuery::eval(text); 
auto ret_lines = std::make_shared<std::set<line_no>>();
auto it = res.begin();

// All Regex:
regex regex_words("[\\w']+");
regex regex_R2L("(.)*" + right_query + " ([\\w']+ ){0," + to_string(dist) +"}" + left_query + "(.)*");
regex regex_L2R("(.)*" + left_query + " ([\\w']+ ){0," + to_string(dist) +"}" + right_query + "(.)*");

for (; it != res.end(); ++it) {
		string line = res.get_file()->at(*it);
		string new_line = "";
		auto start = sregex_iterator(line.begin(), line.end(), regex_words);
		auto end = sregex_iterator();
		for (sregex_iterator i = start; i != end; ++i) {
			smatch Match = *i;
			string Match_str = Match.str();
			new_line = new_line + Match_str + " ";
		}
		if (regex_match(new_line, regex_R2L) || regex_match(new_line, regex_L2R)) 
    {
			ret_lines->insert(*it);
		}
	}
  return QueryResult(rep(), ret_lines, res.get_file());

}
/////////////////////////////////////////////////////////
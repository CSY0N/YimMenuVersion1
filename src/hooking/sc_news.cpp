#include "hooking.hpp"
namespace big
{
	//Credit Devra
	bool hooks::request_story_news_data(CNetworkSCNewsStoryRequest* pStory)
	{
		auto items = pStory->toJson();
		if (!items.empty())
		{
			static int i = 0;
			struct News
			{
				const char* headline;
				const char* subtitle;
				const char* content;
			};
			static const News news[] = {
			{"YimMenu Version", "Legacy Build", "You are currently using the legacy build of YimMenu."}, 
			{"Upgrade Available", "Enhanced Version", "YimMenu V2 is available for Enhanced at https://github.com/CSY0N/YimMenuVersion2"}};
			const auto& n     = news[i++ % 2];
			items["title"]    = "YimMenu";
			items["headline"] = n.headline;
			items["subtitle"] = n.subtitle;
			items["content"]  = n.content;
			pStory->dump(items);
		}
		return g_hooking->m_request_story_news.get_original<decltype(&request_story_news_data)>()(pStory);
	}

}

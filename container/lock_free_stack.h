#pragma once
// 无锁栈
// 2024-6-22

#include <condition_variable>
#include <mutex>
#include <queue>
#include <atomic>
#include <memory>

//最大风险指针数量
unsigned const max_hazard_pointers = 100;
//风险指针
struct hazard_pointer {
	std::atomic<std::thread::id> id;
	std::atomic<void*>  pointer;
};
//风险指针数组
extern hazard_pointer hazard_pointers[max_hazard_pointers];
//风险指针持有类
class hp_owner {
public:
	hp_owner(hp_owner const&) = delete;
	hp_owner operator=(hp_owner const&) = delete;
	hp_owner() :hp(nullptr) {
		bind_hazard_pointer();
	}

	std::atomic<void*>& get_pointer() {
		return hp->pointer;
	}

	~hp_owner() {
		hp->pointer.store(nullptr);
		hp->id.store(std::thread::id());
	}
private:
	void bind_hazard_pointer() {
		for (unsigned i = 0; i < max_hazard_pointers; ++i) {
			std::thread::id  old_id;
			if (hazard_pointers[i].id.compare_exchange_strong(old_id, std::this_thread::get_id())) {
				hp = &hazard_pointers[i];
				break;
			}
		}

		if (!hp) {
			throw std::runtime_error("No hazard pointers available");
		}
	}
	hazard_pointer* hp;
};

std::atomic<void*>& get_hazard_pointer_for_current_thread() {
	//每个线程都具有自己的风险指针 线程本地变量
	thread_local static hp_owner hazzard;
	return hazzard.get_pointer();
}

template<typename T>
class hazard_pointer_stack {
private:

	//栈节点
	struct node {
		std::shared_ptr<T> data;
		node* next;
		node(T const& data_) :data(std::make_shared<T>(data_)) {}
	};

	//待删节点
	struct data_to_reclaim {
		node* data;
		data_to_reclaim* next;
		data_to_reclaim(node* p) :data(p), next(nullptr) {}
		~data_to_reclaim() {
			delete data;
		}
	};

	hazard_pointer_stack(const hazard_pointer_stack&) = delete;
	hazard_pointer_stack& operator = (const hazard_pointer_stack&) = delete;
	std::atomic<node*> head;
	std::atomic<data_to_reclaim*>  nodes_to_reclaim;
public:
	hazard_pointer_stack() {}

	void push(T const& data) {
		node* const new_node = new node(data);    //⇽-- - 2			
		new_node->next = head.load();    //⇽-- - 3			
		while (!head.compare_exchange_weak(new_node->next, new_node));    //⇽-- - 4	
	}

	bool outstanding_hazard_pointers_for(void* p)
	{
		for (unsigned i = 0; i < max_hazard_pointers; ++i)
		{
			if (hazard_pointers[i].pointer.load() == p)
			{
				return true;
			}
		}
		return false;
	}

	void add_to_reclaim_list(data_to_reclaim* reclaim_node) {
		reclaim_node->next = nodes_to_reclaim.load();
		while (!nodes_to_reclaim.compare_exchange_weak(reclaim_node->next, reclaim_node));
	}

	void reclaim_later(node* old_head) {
		add_to_reclaim_list(new data_to_reclaim(old_head));
	}

	void delete_nodes_with_no_hazards() {
		data_to_reclaim* current = nodes_to_reclaim.exchange(nullptr);
		while (current) {
			data_to_reclaim* const next = current->next;
			if (!outstanding_hazard_pointers_for(current->data)) {
				delete current;
			}
			else {
				add_to_reclaim_list(current);
			}

			current = next;
		}
	}

	std::shared_ptr<T> pop() {
		//1 从风险列表中获取一个节点给当前线程
		std::atomic<void*>& hp = get_hazard_pointer_for_current_thread();
		node* old_head = head.load();
		do
		{
			node* temp;
			do
			{
				temp = old_head;
				hp.store(old_head);
				old_head = head.load();
			}//2 如果old_head和temp不等说明head被其他线程更新了，需重试
			while (old_head != temp);
		}//3 将当前head更新为old_head->next，如不满足则重试
		while (old_head &&
			!head.compare_exchange_strong(old_head, old_head->next));
		// 4一旦更新了head指针，便将风险指针清零
		hp.store(nullptr);
		std::shared_ptr<T> res;
		if (old_head)
		{
			res.swap(old_head->data);
			//5 删除旧有的头节点之前，先核查它是否正被风险指针所指涉
			if (outstanding_hazard_pointers_for(old_head))
			{
				//6 延迟删除
				reclaim_later(old_head);
			}
			else
			{
				//7 删除头部节点
				delete old_head;
			}
			//8 删除没有风险的节点
			delete_nodes_with_no_hazards();
		}
		return res;
	}
};

template<typename T>
class lock_free_stack {
private:
	struct node {
		std::shared_ptr<T> data;
		node* next;
		node(T const& data_) :data(std::make_shared<T>(data_)) {}
	};
	lock_free_stack(const lock_free_stack&) = delete;
	lock_free_stack& operator = (const lock_free_stack&) = delete;
	std::atomic<node*> head;
	std::atomic<node*> to_be_deleted;
	std::atomic<int> threads_in_pop;

public:
	lock_free_stack() {}

	void push(T const& data) {
		node* const new_node = new node(data);    //⇽-- - 2			
		new_node->next = head.load();    //⇽-- - 3			
		while (!head.compare_exchange_weak(new_node->next, new_node));    //⇽-- - 4	
	}

	std::shared_ptr<T> pop() {
		++threads_in_pop;   //1 计数器首先自增，然后才执行其他操作
		node* old_head = nullptr;
		do {
			old_head = head.load();  //2 加载head节点给旧head存储
			if (old_head == nullptr) {
				--threads_in_pop;
				return nullptr;
			}
		} while (!head.compare_exchange_weak(old_head, old_head->next)); // 3	比较更新head为旧head的下一个节点	

		std::shared_ptr<T> res;
		if (old_head)
		{
			// 4 只要有可能，就回收已删除的节点数据
			res.swap(old_head->data);
		}
		// 5 从节点提取数据，而非复制指针
		try_reclaim(old_head);
		return res;
	}

	void try_reclaim(node* old_head)
	{
		//1 原子变量判断仅有一个线程进入
		if (threads_in_pop == 1)
		{
			//2 当前线程把待删列表取出
			node* nodes_to_delete = to_be_deleted.exchange(nullptr);
			//3 更新原子变量获取准确状态，判断pop是否仅仅正被当前线程唯一调用
			if (!--threads_in_pop)
			{
				//4 如果唯一调用则将待删列表删除
				delete_nodes(nodes_to_delete);
			}
			else if (nodes_to_delete)
			{
				//5 如果pop还有其他线程调用且待删列表不为空，
				//则将待删列表首节点更新给to_be_deleted
				chain_pending_nodes(nodes_to_delete);
			}
			delete old_head;
		}
		else {
			//多个线程pop竞争head节点，此时不能删除old_head
			//将其放入待删列表
			chain_pending_node(old_head);
			--threads_in_pop;
		}
	}

	static void delete_nodes(node* nodes)
	{
		while (nodes)
		{
			node* next = nodes->next;
			delete nodes;
			nodes = next;
		}
	}

	void chain_pending_node(node* n)
	{
		chain_pending_nodes(n, n);
	}

	void chain_pending_nodes(node* first, node* last)
	{
		//1 先将last的next节点更新为待删列表的首节点
		last->next = to_be_deleted;
		//2  借循环保证 last->next指向正确
		// 将待删列表的首节点更新为first节点
		while (!to_be_deleted.compare_exchange_weak(
			last->next, first));
	}

	void chain_pending_nodes(node* nodes)
	{
		node* last = nodes;
		//1 沿着next指针前进到链表末端
		while (node* const next = last->next)
		{
			last = next;
		}
		//2 将链表放入待删链表中
		chain_pending_nodes(nodes, last);
	}
};
#include "LinkList.h"


Node* CreateEmptyList()
{
	Node* head;

	head =(Node*) malloc(sizeof(Node));
	head->next = NULL;
	head->prev = NULL;

	return head;
}


Node* CreateList(DataType *addr, unsigned int n)
{
	Node *head;

	head = (Node*) malloc(sizeof(Node));
	head->next = NULL;
	head->prev = NULL;

	for (unsigned int i = 0; i < n; i++)
	{
		ListInsert(head, i, addr[i]);
	}

	return head;
}


void DestroyList(Node *head)
{
	if (head == NULL) return;

	Node *p;

	while (head->next != NULL)
	{
		p = head->next;
		free(head);
		head = p;
	}
	free(head);
}


void ListInsert(Node *head, unsigned int index, DataType data)
{
	unsigned int j = 0;
	Node* p = head;

	while (p && j < index)
	{
		p = p->next;
		++j;
	}
	if (p == NULL)
		return;

	Node *s = (Node*)malloc(sizeof(Node));
	s->data = data;
	s->next = p->next;
	s->prev = p;
	if (p->next != NULL)
		p->next->prev = s;
	p->next = s;
}


DataType ListDelete(Node *head, unsigned int index)
{
	unsigned int j = 0;
	Node *p = head;
	Node* q;
	DataType data;

	while (p->next && j < index)
	{
		p = p->next;
		++j;
	}

	q = p->next;
	if (q == NULL) {
		DataType empty = {0, 0, 0.0f, 0.0f, (BallColor)0};
		return empty;
	}
	p->next = q->next;
	if (q->next != NULL)
		q->next->prev = p;
	data = q->data;
	free(q);

	return data;
}


int EliminateRuns(Node *head)
{
	int totalScore = 0;
	int chainLevel = 0;

	Node *curr = head->next;
	if (curr == NULL) return 0;

	while (curr != NULL && curr->next != NULL) {
		BallColor color = curr->data.c;
		int count = 1;
		Node *runEnd = curr;

		while (runEnd->next != NULL && runEnd->next->data.c == color) {
			runEnd = runEnd->next;
			count++;
		}

		if (count >= 3) {
			Node *before = curr->prev;
			Node *after  = runEnd->next;

			Node *p = curr;
			while (p != after) {
				Node *next = p->next;
				free(p);
				p = next;
			}

			before->next = after;
			if (after != NULL)
				after->prev = before;

			chainLevel++;
			int baseScore = count * count * 10;
			double chainMul = 1.0 + (chainLevel - 1) * 0.5;
			totalScore += (int)(baseScore * chainMul);

			curr = before;
			if (curr == head) {
				curr = head->next;
			} else {
				while (curr->prev != head && curr->prev->data.c == curr->data.c)
					curr = curr->prev;
			}
		} else {
			curr = runEnd->next;
		}
	}

	return totalScore;
}

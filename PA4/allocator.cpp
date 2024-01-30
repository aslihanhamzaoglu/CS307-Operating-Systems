#include <iostream>
#include <pthread.h>
#include <unistd.h>
using namespace std;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

class HeapManager {

public:

    int initHeap(int size) {
        
        head = new node(-1, size, 0, NULL);
        cout << "Memory initialized " << endl;
        print();

        return 1;
    }

    int myMalloc(int ID, int size) {
        
        pthread_mutex_lock(&lock);
        int index;
        node* temp = NULL;
        bool fit = false;
        if (head->size >= size && head->id == -1) { //head is the first fit

            index = 0;
            if (head->size == size)
                head->id = ID;
            else {

                temp = head->next;
                head->next = new node(-1, head->size - size, size, NULL);
                head->size = size;
                head->next->next = temp;
                head->id = ID;
            }
            cout << "Allocated for thread " << ID << endl;
            print();
        }       
        else {  //first fit doesn't exist OR exist but it is not the head
            node* prev = head;

            while (prev->next != NULL && !fit)
            {
                temp = prev->next;
                if (temp->size >= size && temp->id == -1) {
                    fit = true;
                }
                prev = prev->next;
            }

            if( !fit){
                
                cout << "Can not allocate, requested size "<< size <<" for thread "<<  ID <<" is bigger than remaining size" << endl;
                print();
                pthread_mutex_unlock(&lock);
                return -1;
            }
            else {

                index = temp->index;
                if (temp->size == size) {
                    temp->id = ID;
                }
                else {

                    node* next;
                    next = temp->next;
                    temp->next = new node(-1, temp->size - size, (temp->index + size), NULL);
                    temp->size = size;
                    temp->id = ID;
                    temp->next->next = next;
                }
                cout << "Allocated for thread " << ID << endl;
                print();
            }

        }
        pthread_mutex_unlock(&lock);
        return index;
    }

    int myFree(int ID, int index) {
        
        pthread_mutex_lock(&lock);
        node* temp = head;

        if (head->id == ID && head->index == index) { //HEAD IS TO BE FREED INDEX

            head->id = -1;
            temp = head->next;

            if (temp->id == -1) {

                head->size += temp->size;
                head->next = temp->next;
                delete temp;
            }

            cout << "Freed for thread " << ID << endl;
            print();
            pthread_mutex_unlock(&lock);
            return 1;
        }
        else { //HEAD IS NOT TO BE FREED INDEX

            while (temp->next != NULL) {

                if (temp->next->id == ID && temp->next->index == index) {

                    node* ptr = temp->next;
                    temp->next->id = -1;

                    if (ptr->next->id == -1) { //NEXT NODE IS CHECKED TO COMBINE

                        ptr->size += ptr->next->size;
                        node* toBeDeleted = ptr->next;
                        ptr->next = toBeDeleted->next;
                        delete toBeDeleted;
                        
                    }
                    if (temp->id == -1) {   //PREVIOUS NODE IS CHECKED TO COMBINE
                        temp->size += ptr->size;
                        temp->next = ptr->next;
                        delete ptr;
                    }

                    cout << "Freed for thread " << ID << endl;
                    print();
                    pthread_mutex_unlock(&lock);
                    return 1;
                }
                temp = temp->next;
            }
        }
        pthread_mutex_unlock(&lock);
        return -1;
    }

    void print() const {
        
        cout << "["<< head->id << "]" << "[" << head->size << "]" << "[" << head->index << "]";

        node* temp = head;

        while (temp->next != NULL) {

            temp = temp->next;
            cout << "---[" << temp->id << "]" << "[" << temp->size << "]" << "[" << temp->index << "]";
        }
        cout << endl;
    }

private:

    struct node {

        int id;
        int size;
        int index;

        node* next;

        node (int i, int s, int st, node* n) : id(i), size(s), index(st), next(n)
        {}
    };

    node* head;
};
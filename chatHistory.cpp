#include <iostream>
#include <ctime>
#include <cstring>

using namespace std;


struct Msg {
    char text[256];
    time_t ts;
    Msg* prev;
    Msg* next;
};


struct Queue {
    Msg* head;
    Msg* tail;
};

struct Stack {
    Msg* top;
};

Queue q = {nullptr, nullptr};
Stack undoStack = {nullptr};
Stack redoStack = {nullptr};

void input_line(const char* prompt, char* buf, size_t n){
    cout << prompt;
    cin.getline(buf, (int)n);
    buf[n-1] = '\0';
}

Msg* new_msg(const char* text){
    Msg* m = new Msg;
    strncpy(m->text, text, sizeof(m->text)); m->text[sizeof(m->text)-1]='\0';
    m->ts = time(nullptr);
    m->prev = m->next = nullptr;
    return m;
}


void q_push(Queue& q, Msg* m){
    m->prev = q.tail;
    m->next = nullptr;
    if(!q.tail){ q.head = q.tail = m; }
    else { q.tail->next = m; q.tail = m; }
}
Msg* q_pop_tail(Queue& q){ // remove most recent message (for undo)
    if(!q.tail) return nullptr;
    Msg* t = q.tail;
    q.tail = t->prev;
    if(q.tail) q.tail->next = nullptr;
    else q.head = nullptr;
    t->prev = t->next = nullptr;
    return t;
}


void st_push(Stack& s, Msg* m){
    m->next = nullptr;
    m->prev = s.top;
    s.top = m;
}
Msg* st_pop(Stack& s){
    if(!s.top) return nullptr;
    Msg* t = s.top;
    s.top = t->prev;
    t->prev = t->next = nullptr;
    return t;
}

void print_time(time_t t){
    tm *lt = localtime(&t);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt);
    cout << buf;
}

void send_message(){
    char txt[256]; input_line("Type message: ", txt, sizeof(txt));
    Msg* m = new_msg(txt);
    q_push(q, m);
    st_push(undoStack, m);   // track for undo
    // clearing redo on new action
    while(redoStack.top){ Msg* x = st_pop(redoStack); delete x; }
    cout << "Sent.\n";
}

void view_history(){
    if(!q.head){ cout << "No messages.\n"; return; }
    cout << "=== Chat History (oldest -> newest) ===\n";
    int i=1;
    for(Msg* p=q.head; p; p=p->next, ++i){
        cout << i << ". [";
        print_time(p->ts);
        cout << "] " << p->text << "\n";
    }
}

void undo_last(){
    Msg* m = q_pop_tail(q);
    if(!m){ cout << "Nothing to undo.\n"; return; }
    Msg* u = st_pop(undoStack);
    (void)u;
    st_push(redoStack, m);
    cout << "Undo: removed last message.\n";
}

void redo_last(){
    
    Msg* m = st_pop(redoStack);
    if(!m){ cout << "Nothing to redo.\n"; return; }
    q_push(q, m);
    st_push(undoStack, m);
    cout << "Redo: message restored.\n";
}

void save_to_file(const char* fname){
    FILE* fp = fopen(fname, "wb");
    if(!fp){ cout << "Cannot open file.\n"; return; }
    for(Msg* p=q.head; p; p=p->next){
        fwrite(&p->ts, sizeof(p->ts), 1, fp);
        fwrite(p->text, sizeof(p->text), 1, fp);
    }
    fclose(fp);
    cout << "Saved to " << fname << "\n";
}

void load_from_file(const char* fname){
    while(q.tail){ Msg* x = q_pop_tail(q); delete x; }
    while(undoStack.top){ Msg* x = st_pop(undoStack); delete x; }
    while(redoStack.top){ Msg* x = st_pop(redoStack); delete x; }

    FILE* fp = fopen(fname, "rb");
    if(!fp){ cout << "No save file.\n"; return; }
    while(true){
        time_t ts;
        char text[256];
        if(fread(&ts, sizeof(ts), 1, fp)!=1) break;
        if(fread(text, sizeof(text), 1, fp)!=1) break;
        Msg* m = new Msg;
        strncpy(m->text, text, sizeof(m->text)); m->text[sizeof(m->text)-1]='\0';
        m->ts = ts; m->prev=m->next=nullptr;
        q_push(q, m);
        st_push(undoStack, m);
    }
    fclose(fp);
    cout << "Loaded from " << fname << "\n";
}

void cleanup(){
    while(q.tail){ Msg* x = q_pop_tail(q); delete x; }
    while(undoStack.top){ Msg* x = st_pop(undoStack); delete x; }
    while(redoStack.top){ Msg* x = st_pop(redoStack); delete x; }
}

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    const char* FILE_NAME = "chat_history.dat";
    while(true){
        cout << "\n=== Chat Message History Manager ===\n"
             << "1. Send Message\n2. View History\n3. Undo Last\n4. Redo Last\n"
             << "5. Save\n6. Load\n7. Exit\nChoice: ";
        int ch; if(!(cin>>ch)) return 0; cin.ignore(1024,'\n');

        if(ch==1) send_message();
        else if(ch==2) view_history();
        else if(ch==3) undo_last();
        else if(ch==4) redo_last();
        else if(ch==5) save_to_file(FILE_NAME);
        else if(ch==6) load_from_file(FILE_NAME);
        else if(ch==7){ cleanup(); cout << "Goodbye!\n"; break; }
        else cout << "Invalid.\n";
    }
    return 0;
}

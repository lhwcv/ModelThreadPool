#ifndef MODEL_THREAD_POOL_H
#define MODEL_THREAD_POOL_H

#include <vector>
#include <queue>
#include <list>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <sstream>

namespace hpc
{
    typedef struct {
        float x0, y0, x1, y1, x2, y2, x3, y3;
        std::string text;
        float score;
    } TextLineResult;

    typedef struct {
        unsigned char *data;
        int w, h;
    } Image;

    class Model {
    private:
        int _modelID;

    public:
        Model(int modelID) : _modelID(modelID) {}

        virtual int load_model(std::vector<std::string> &modelCfg) {return  0;}

        TextLineResult recog_text_line(Image image)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            TextLineResult r;
            std::stringstream ss;
            ss <<"w: "<<image.w<<",h:  "<<image.h<< "-- I am model " << _modelID;
            //std::cout<<ss.str()<<std::endl;
            r.text = ss.str();
            return r;
        }
    };

    class Model_ThreadPool {
    public:
        explicit Model_ThreadPool(size_t threads);
        //TODO
        int load_models(std::vector<std::vector<std::string>> &modelCfgs)
        {
            if(modelCfgs.size() != models.size())
            {
                throw std::runtime_error("modelCgs.size() != models.size()");
            }
            for(size_t i=0; i < modelCfgs.size(); i++)
            {
                models[i].load_model(modelCfgs[i]);
            }
            return 0;
        }

        auto enqueue(Image image) -> std::future<TextLineResult>;
        ~Model_ThreadPool();

    private:
        std::vector<std::thread> workers;
        std::vector<Model> models;
        std::list<std::packaged_task<TextLineResult(Model &)>> tasks;
        std::mutex queue_mutex;
        std::condition_variable condition;
        bool stop;
    };

    inline Model_ThreadPool::Model_ThreadPool(size_t threads)
            : stop(false) {
        for (size_t i = 0; i < threads; ++i) {
            models.emplace_back(Model(i));
            workers.emplace_back([this, i] {
                Model &model = models[i];
                while (true) {
                    std::packaged_task<TextLineResult(Model &)> task;
                    {
                        //std::cout<<"wait here"<<std::endl;
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });

                        if (this->stop && this->tasks.empty()) return;

                        task = std::move(this->tasks.front());
                        this->tasks.pop_front();
                    }
                    task(model);
                }
            });
        }
    }

    auto Model_ThreadPool::enqueue(Image image) -> std::future<TextLineResult> {
        auto task_function = [image](Model &model) { return model.recog_text_line(image); };
        std::packaged_task<TextLineResult(Model &)> task(std::move(task_function));

        std::future<TextLineResult> res = task.get_future();

        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            if (stop) throw std::runtime_error("enqueue on stopped Model_ThreadPool");

            tasks.emplace_back(std::move(task));
        }
        condition.notify_one();
        return res;
    }

   // the destructor joins all threads
    inline Model_ThreadPool::~Model_ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers)
            worker.join();
    }


}//namespace

#endif //MODEL_THREAD_POOL_H

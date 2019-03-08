#include "tensorflow/cc/client/client_session.h"
#include "tensorflow/cc/ops/standard_ops.h"
#include "tensorflow/cc/framework/gradients.h"

int main()
{
    // benchmark
    auto benchmark_w = 2.0, benchmark_b = 0.5;

    // data
    auto nof_samples = 100;
    struct Sample 
    {
        float sample;
        float label;
    };
    std::vector<struct Sample> dataset;
    std::srand((unsigned)std::time(NULL));
    for (int i = 0; i < nof_samples; i++)
    {
        float sample = std::rand() / float(RAND_MAX) - 0.5;
        float label = benchmark_w * sample + benchmark_b + std::rand() / float(RAND_MAX) * 0.01;
        dataset.push_back({sample, label});
    }

    // model
    tensorflow::Scope root = tensorflow::Scope::NewRootScope();
    auto x = tensorflow::ops::Placeholder(root, tensorflow::DataType::DT_FLOAT);
    auto y = tensorflow::ops::Placeholder(root, tensorflow::DataType::DT_FLOAT);
    auto w = tensorflow::ops::Variable(root, {1, 1}, tensorflow::DataType::DT_FLOAT);
    auto assign_w = tensorflow::ops::Assign(root, w, tensorflow::ops::RandomNormal(root, {1, 1}, tensorflow::DataType::DT_FLOAT));
    auto b = tensorflow::ops::Variable(root, {1, 1}, tensorflow::DataType::DT_FLOAT);
    auto assign_b = tensorflow::ops::Assign(root, b, {{0.0f}});
    auto y_ = tensorflow::ops::Add(root, tensorflow::ops::MatMul(root, x, w), b);
    auto loss = tensorflow::ops::L2Loss(root, tensorflow::ops::Sub(root, y_, y));
    std::vector<tensorflow::Output> grad_outputs;
    TF_CHECK_OK(AddSymbolicGradients(root, {loss}, {w, b}, &grad_outputs));
    auto learn_rate = tensorflow::ops::Const(root, 0.01f, {});
    auto apply_w = tensorflow::ops::ApplyGradientDescent(root, w, learn_rate, {grad_outputs[0]});
    auto apply_b = tensorflow::ops::ApplyGradientDescent(root, b, learn_rate, {grad_outputs[1]});

    // train
    tensorflow::ClientSession sess(root);
    sess.Run({assign_w, assign_b}, nullptr);
    std::vector<tensorflow::Tensor> outputs;
    timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int epoch = 1; epoch <= 64; epoch++)
    {
        std::random_shuffle(dataset.begin(), dataset.end());
        for (int i = 0; i < nof_samples; i++)
        {
            TF_CHECK_OK(sess.Run({{x, {{dataset[i].sample}}}, {y, {{dataset[i].label}}}}, {w, b, loss, apply_w, apply_b}, &outputs));
        }
        LOG(INFO) << "epoch " << epoch << ": w=" << outputs[0].matrix<float>() << " b=" << outputs[1].matrix<float>() << " loss=" << outputs[2].scalar<float>();
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    LOG(INFO) << "elapsed time： " << t1.tv_sec - t0.tv_sec + (t1.tv_nsec - t0.tv_nsec) * 1.0 / 1000000000 << "s";
    return 0;
}

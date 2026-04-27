#pragma once

namespace rain{
    enum class node_status
    {
        success,
        failure,
        running
    };

    class tree_context
    {
    public:
    };

    class tree_node
    {
    public:
        virtual ~tree_node() = default;

        virtual node_status tick(tree_context& context) = 0;

        virtual void reset()
        {
        }
    };

    class control_node : public tree_node
    {
    public:
        void add_child(std::unique_ptr<tree_node> child)
        {
            children.push_back(std::move(child));
        }

    protected:
        std::vector<std::unique_ptr<tree_node>> children;
    };

    class sequence_node : public control_node
    {
    public:
        node_status tick(tree_context& context) override;
        void reset() override;

    private:
        size_t current_index = 0;
    };

    class selector_node : public control_node
    {
    public:
        node_status tick(tree_context& context) override;
        void reset() override;

    private:
        size_t current_index = 0;
    };

    class leaf_node : public tree_node
    {
    };

    class action_node : public leaf_node
    {
    public:
        using node_func = std::function<node_status(tree_context&)>;

    public:
        explicit action_node(node_func input_func)
            : func(std::move(input_func))
        {
        }

        node_status tick(tree_context& context) override
        {
            return func(context);
        }

    private:
        node_func func;
    };

    class condition_node : public leaf_node
    {
    public:
        using node_func = std::function<BOOL(tree_context&)>;

    public:
        explicit condition_node(node_func input_func)
            : func(std::move(input_func))
        {
        }

        node_status tick(tree_context& context) override
        {
            return func(context) ? node_status::success : node_status::failure;
        }

    private:
        node_func func;
    };
}
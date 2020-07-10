// State of application
const state = {}
const getKeyByValue = (value) =>
    Object.keys(api).find((key) => api[key] === value)

const request = async (method, url, payload) => {
    // Create basic header
    const headers = {
        method: method,
        headers: {
            'Content-Type': 'application/json',
        },
    }
    if (payload) headers.body = JSON.stringify(payload) // payload for PUT and POST requests
    const fetchedData = await fetch(url, headers)
    const key = getKeyByValue(url)

    // if request recieved or parsed with errors set state undefined
    const json = await fetchedData.json().catch(() => {
        state[key] = {
            data: undefined,
            status: false,
        }
    })

    if (json) state[key] = { data: json, status: true } // if ok, set data

    // reactive set states in components after request
    setState(key)
    return json
}
const Request = {
    Get: async (url) => await request('GET', url), // GET Request
    Put: async (url, payload) => await request('PUT', url, payload), // PUT Request
    Post: async (url, payload) => await request('POST', url, payload), // POST Request
}

// Work with DOM
const El = {
    ById: (id) => document.getElementById(id), // El.ById(id) - get element by id
    ByName: {
        One: (name) => document.getElementsByName(name)[0], // El.ByName.One(name) - get element by name
        All: (name) => document.getElementsByName(name), // El.ByName.All(name) - get elements by name
    },
    Create: {
        El: (name, opts) => {
            if (opts) {
                const el = document.createElement(name)
                const keys = Object.keys(opts)
                keys.forEach((key) => (el[key] = opts[key]))
                return el
            }
            return document.createElement(name)
        }, // El.Create.El(name, opts) - name required, opts optional. Create element and applying options
        Text: (text) => document.createTextNode(text), // El.Create.Text(text) - create text node
        Label: (text) => {
            const el = El.Create.El('label')
            el.innerHTML = text
            return el
        },
        Select: (items, value) => {
            const el = El.Create.El('select')
            items.forEach ((item) => {
                const option = El.Create.El('option',{
                    value: item,
                    text: item,
                })
                El.Add(el, option)
            })
            el.value = value
            return el
        },
    },
    Add: (parent, child) => parent.appendChild(child), // El.Add(parent, child) - added child to parent
    AddSeq: (parent, childs) =>
        childs.forEach((child) => El.Add(parent, child)), // El.AddSeq(parent, childs) - childs is array. Added childs to parent by array sequence
}